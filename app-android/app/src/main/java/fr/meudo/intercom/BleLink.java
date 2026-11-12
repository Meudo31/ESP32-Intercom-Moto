package fr.meudo.intercom;

import android.annotation.SuppressLint;
import android.bluetooth.*;
import android.bluetooth.le.*;
import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import org.json.JSONObject;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Consumer;

/** Liaison BLE partagée : une opération GATT à la fois sur un thread dédié ; événements remis sur le thread UI. */
@SuppressLint("MissingPermission")   // les permissions sont demandées par MainActivity avant connect()
final class BleLink {

    static final int DISCONNECTED = 0, SCANNING = 1, CONNECTING = 2, CONNECTED = 3;

    interface Listener {
        void onState(int state, String message);
        void onData(byte[] packet);
    }

    interface Progress {
        void onProgress(long sent, long total);
    }

    private static BleLink instance;

    static synchronized BleLink get(Context c) {
        if (instance == null) instance = new BleLink(c.getApplicationContext());
        return instance;
    }

    private final Context ctx;
    private final Handler ui = new Handler(Looper.getMainLooper());
    private final ExecutorService gattThread = Executors.newSingleThreadExecutor();
    private final List<Listener> listeners = new CopyOnWriteArrayList<>();

    private volatile BluetoothGatt gatt;
    private volatile int state = DISCONNECTED;
    private volatile String stateMessage = "";
    private volatile boolean wanted = false;       // l'utilisateur veut rester connecté
    private volatile long reconnectDelay = 2000;
    private volatile int mtu = 23;
    private volatile JSONObject info = new JSONObject();
    private volatile boolean busy = false;          // une connexion est en cours de mise en place

    private BluetoothGattCharacteristic chData, chMedia, chInfo, chCommand, chOtaCtrl, chOtaData;

    // Résultat de l'opération GATT en cours
    private final Semaphore opDone = new Semaphore(0);
    private volatile int opStatus;
    private volatile byte[] opValue;
    private final LinkedBlockingQueue<String> otaReplies = new LinkedBlockingQueue<>();

    private BleLink(Context ctx) {
        this.ctx = ctx;
    }

    // ---------------------------------------------------------------- API

    void addListener(Listener l) {
        listeners.add(l);
        l.onState(state, stateMessage);
    }

    void removeListener(Listener l) { listeners.remove(l); }

    int state() { return state; }
    JSONObject info() { return info; }
    int mtu() { return mtu; }

    BluetoothAdapter adapter() {
        BluetoothManager m = (BluetoothManager) ctx.getSystemService(Context.BLUETOOTH_SERVICE);
        return m != null ? m.getAdapter() : null;
    }

    void connect() {
        wanted = true;
        if (state != DISCONNECTED || busy) return;
        busy = true;
        gattThread.execute(this::doConnect);
    }

    void disconnect() {
        wanted = false;
        ui.removeCallbacks(reconnect);
        gattThread.execute(() -> {
            closeGatt();
            setState(DISCONNECTED, "Déconnecté.");
        });
    }

    /** Envoie une commande texte ("cle=valeur"). Erreur remise sur le thread UI, null si OK. */
    void sendCommand(String text, Consumer<String> done) {
        gattThread.execute(() -> {
            String err = null;
            try {
                requireConnected();
                write(chCommand, text.getBytes(StandardCharsets.UTF_8));
            } catch (IOException e) {
                err = e.getMessage();
            }
            final String fe = err;
            ui.post(() -> done.accept(fe));
        });
    }

    /** Lit le JSON MEDIA (téléphone et morceau). Résultat sur le thread UI, null en cas d'échec. */
    void readMedia(Consumer<JSONObject> done) {
        gattThread.execute(() -> {
            JSONObject j = null;
            try {
                requireConnected();
                j = new JSONObject(new String(read(chMedia), StandardCharsets.UTF_8));
            } catch (Exception ignored) {
                // la prochaine notification relancera la lecture
            }
            final JSONObject fj = j;
            ui.post(() -> done.accept(fj));
        });
    }

    /**
     * Envoie le firmware (bloquant : à appeler hors du thread UI). Renvoie quand
     * l'ESP32 a vérifié l'image et va redémarrer ; IOException avec la raison sinon.
     */
    void otaUpload(ContentResolver cr, Uri uri, long size, String md5, String key, Progress progress)
            throws IOException, InterruptedException {
        try {
            gattThread.submit(() -> {
                doOta(cr, uri, size, md5, key, progress);
                return null;
            }).get();
        } catch (ExecutionException e) {
            Throwable c = e.getCause();
            if (c instanceof IOException) throw (IOException) c;
            throw new IOException(c != null ? c.getMessage() : "erreur inconnue");
        }
    }

    // ---------------------------------------------------------------- connexion

    private final Runnable reconnect = this::connect;

    private void doConnect() {
        try {
            BluetoothAdapter ad = adapter();
            if (ad == null) throw new IOException("ce téléphone n'a pas de Bluetooth");
            if (!ad.isEnabled()) throw new IOException("le Bluetooth est désactivé");

            setState(SCANNING, "Recherche de l'intercom…");
            BluetoothDevice dev = scan(ad, 10000);
            if (dev == null) throw new IOException("intercom introuvable. Est-il allumé (pas en veille), et à moins de 10 m ?");

            setState(CONNECTING, "Connexion…");
            opDone.drainPermits();
            gatt = dev.connectGatt(ctx, false, callback, BluetoothDevice.TRANSPORT_LE);
            waitOp(15000, "connexion");

            mtu = 23;
            opDone.drainPermits();
            if (gatt.requestMtu(517)) {
                try { waitOp(5000, "MTU"); } catch (IOException ignored) { /* on garde 23 */ }
            }
            gatt.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH);

            opDone.drainPermits();
            if (!gatt.discoverServices()) throw new IOException("découverte des services refusée");
            waitOp(10000, "découverte des services");

            BluetoothGattService svc = gatt.getService(BleProtocol.SERVICE);
            if (svc == null) throw new IOException("cet appareil n'a pas le service attendu (ancien firmware ?)");
            chData = svc.getCharacteristic(BleProtocol.DATA);
            chMedia = svc.getCharacteristic(BleProtocol.MEDIA);
            chInfo = svc.getCharacteristic(BleProtocol.INFO);
            chCommand = svc.getCharacteristic(BleProtocol.COMMAND);
            chOtaCtrl = svc.getCharacteristic(BleProtocol.OTA_CTRL);
            chOtaData = svc.getCharacteristic(BleProtocol.OTA_DATA);
            if (chData == null || chMedia == null || chInfo == null || chCommand == null
                    || chOtaCtrl == null || chOtaData == null)
                throw new IOException("service incomplet (firmware incompatible)");

            enableNotify(chOtaCtrl);
            enableNotify(chData);
            try {
                info = new JSONObject(new String(read(chInfo), StandardCharsets.UTF_8));
            } catch (org.json.JSONException e) {
                info = new JSONObject();
            }
            reconnectDelay = 2000;
            setState(CONNECTED, "Connecté — firmware " + info.optString("fw", "?"));
        } catch (Exception e) {
            closeGatt();
            setState(DISCONNECTED, "❌ " + e.getMessage());
            scheduleReconnect();
        } finally {
            busy = false;
        }
    }

    private BluetoothDevice scan(BluetoothAdapter ad, long timeoutMs) throws InterruptedException {
        BluetoothLeScanner sc = ad.getBluetoothLeScanner();
        if (sc == null) return null;
        final AtomicReference<BluetoothDevice> found = new AtomicReference<>();
        final CountDownLatch latch = new CountDownLatch(1);
        ScanCallback cb = new ScanCallback() {
            @Override public void onScanResult(int type, ScanResult r) {
                found.compareAndSet(null, r.getDevice());
                latch.countDown();
            }
            @Override public void onScanFailed(int errorCode) { latch.countDown(); }
        };
        List<ScanFilter> filters = Collections.singletonList(
                new ScanFilter.Builder().setServiceUuid(new ParcelUuid(BleProtocol.SERVICE)).build());
        ScanSettings settings = new ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build();
        sc.startScan(filters, settings, cb);
        try {
            latch.await(timeoutMs, TimeUnit.MILLISECONDS);
        } finally {
            try { sc.stopScan(cb); } catch (Exception ignored) { /* Bluetooth coupé entre-temps */ }
        }
        return found.get();
    }

    private void scheduleReconnect() {
        if (!wanted) return;
        ui.removeCallbacks(reconnect);
        ui.postDelayed(reconnect, reconnectDelay);
        reconnectDelay = Math.min(reconnectDelay * 2, 15000);
    }

    private void closeGatt() {
        BluetoothGatt g = gatt;
        gatt = null;
        if (g != null) {
            try { g.disconnect(); } catch (Exception ignored) { }
            try { g.close(); } catch (Exception ignored) { }
        }
    }

    private void setState(int s, String msg) {
        state = s;
        stateMessage = msg;
        ui.post(() -> { for (Listener l : listeners) l.onState(s, msg); });
    }

    private void requireConnected() throws IOException {
        if (state != CONNECTED || gatt == null) throw new IOException("intercom non connecté");
    }

    // ---------------------------------------------------------------- opérations GATT (thread gattThread)

    private void waitOp(long ms, String what) throws IOException {
        boolean ok;
        try {
            ok = opDone.tryAcquire(ms, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            throw new IOException("interrompu");
        }
        if (!ok) throw new IOException("pas de réponse de l'intercom (" + what + ")");
        if (opStatus != BluetoothGatt.GATT_SUCCESS) throw new IOException("échec Bluetooth " + opStatus + " (" + what + ")");
    }

    @SuppressWarnings("deprecation")
    private void enableNotify(BluetoothGattCharacteristic c) throws IOException {
        gatt.setCharacteristicNotification(c, true);
        BluetoothGattDescriptor d = c.getDescriptor(BleProtocol.CCCD);
        if (d == null) throw new IOException("notifications indisponibles");
        d.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
        opDone.drainPermits();
        if (!gatt.writeDescriptor(d)) throw new IOException("abonnement refusé");
        waitOp(5000, "abonnement");
    }

    private byte[] read(BluetoothGattCharacteristic c) throws IOException {
        BluetoothGatt g = gatt;
        if (g == null) throw new IOException("intercom déconnecté");
        opDone.drainPermits();
        opValue = null;
        if (!g.readCharacteristic(c)) throw new IOException("lecture refusée");
        waitOp(5000, "lecture");
        return opValue != null ? opValue : new byte[0];
    }

    @SuppressWarnings("deprecation")
    private void write(BluetoothGattCharacteristic c, byte[] value) throws IOException {
        BluetoothGatt g = gatt;
        if (g == null) throw new IOException("intercom déconnecté");
        c.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);   // avec réponse = contrôle de flux
        c.setValue(value);
        opDone.drainPermits();
        boolean started = false;
        for (int i = 0; i < 20 && !started; i++) {
            started = g.writeCharacteristic(c);
            if (!started) sleep(10);   // pile Bluetooth encore occupée
        }
        if (!started) throw new IOException("écriture refusée");
        waitOp(5000, "écriture");
    }

    private static void sleep(long ms) {
        try { Thread.sleep(ms); } catch (InterruptedException ignored) { Thread.currentThread().interrupt(); }
    }

    // ---------------------------------------------------------------- mise à jour

    private void doOta(ContentResolver cr, Uri uri, long size, String md5, String key, Progress progress)
            throws IOException, InterruptedException {
        requireConnected();
        otaReplies.clear();
        write(chOtaCtrl, ("BEGIN " + size + " " + md5 + " " + key).getBytes(StandardCharsets.UTF_8));
        expect("OK", 10000);

        int chunk = Math.max(20, Math.min(mtu - 3, 512));
        long sent = 0;
        try (InputStream in = cr.openInputStream(uri)) {
            if (in == null) throw new IOException("fichier illisible");
            byte[] buf = new byte[chunk];
            while (sent < size) {
                int n = readFully(in, buf);
                if (n <= 0) break;
                byte[] part = n == buf.length ? buf.clone() : Arrays.copyOf(buf, n);
                write(chOtaData, part);
                sent += n;
                String err = otaReplies.peek();
                if (err != null && err.startsWith("ERR")) throw new IOException(esp(otaReplies.poll()));
                progress.onProgress(sent, size);
            }
        } catch (IOException e) {
            try { write(chOtaCtrl, "ABORT".getBytes(StandardCharsets.UTF_8)); } catch (IOException ignored) { }
            throw e;
        }
        if (sent != size) throw new IOException("fichier plus court que prévu");
        write(chOtaCtrl, "END".getBytes(StandardCharsets.UTF_8));
        reconnectDelay = 5000;   // l'ESP32 redémarre : on lui laisse le temps avant de se reconnecter
        expect("DONE", 30000);
    }

    private void expect(String wanted, long ms) throws IOException, InterruptedException {
        String r = otaReplies.poll(ms, TimeUnit.MILLISECONDS);
        if (r == null) throw new IOException("pas de réponse de l'intercom");
        if (!r.equals(wanted)) throw new IOException(esp(r));
    }

    private static String esp(String reply) {
        if (reply == null) return "erreur inconnue";
        return "l'intercom a refusé : " + (reply.startsWith("ERR ") ? reply.substring(4) : reply);
    }

    private static int readFully(InputStream in, byte[] buf) throws IOException {
        int off = 0;
        while (off < buf.length) {
            int n = in.read(buf, off, buf.length - off);
            if (n < 0) break;
            off += n;
        }
        return off;
    }

    // ---------------------------------------------------------------- callbacks Android

    private void onNotify(UUID id, byte[] v) {
        if (v == null) return;
        if (BleProtocol.OTA_CTRL.equals(id)) {
            otaReplies.offer(new String(v, StandardCharsets.UTF_8));
        } else if (BleProtocol.DATA.equals(id)) {
            final byte[] copy = v.clone();
            ui.post(() -> { for (Listener l : listeners) l.onData(copy); });
        }
    }

    private void opFinished(int status, byte[] value) {
        opStatus = status;
        opValue = value;
        opDone.release();
    }

    private final BluetoothGattCallback callback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt g, int status, int newState) {
            if (newState == BluetoothProfile.STATE_CONNECTED && status == BluetoothGatt.GATT_SUCCESS) {
                opFinished(BluetoothGatt.GATT_SUCCESS, null);
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                boolean wasConnected = state == CONNECTED;
                opFinished(status == BluetoothGatt.GATT_SUCCESS ? -1 : status, null);  // débloque l'opération en cours
                otaReplies.offer("ERR liaison Bluetooth perdue");
                if (wasConnected) {
                    gattThread.execute(() -> {
                        closeGatt();
                        setState(DISCONNECTED, "Liaison perdue, reconnexion…");
                        scheduleReconnect();
                    });
                }
            }
        }

        @Override public void onMtuChanged(BluetoothGatt g, int newMtu, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) mtu = newMtu;
            opFinished(status, null);
        }

        @Override public void onServicesDiscovered(BluetoothGatt g, int status) { opFinished(status, null); }

        @Override public void onDescriptorWrite(BluetoothGatt g, BluetoothGattDescriptor d, int status) {
            opFinished(status, null);
        }

        @Override public void onCharacteristicWrite(BluetoothGatt g, BluetoothGattCharacteristic c, int status) {
            opFinished(status, null);
        }

        // Android 13+ : la valeur arrive en paramètre
        @Override public void onCharacteristicRead(BluetoothGatt g, BluetoothGattCharacteristic c, byte[] value, int status) {
            opFinished(status, value);
        }

        // Android 12 et avant
        @SuppressWarnings("deprecation")
        @Override public void onCharacteristicRead(BluetoothGatt g, BluetoothGattCharacteristic c, int status) {
            opFinished(status, c.getValue());
        }

        @Override public void onCharacteristicChanged(BluetoothGatt g, BluetoothGattCharacteristic c, byte[] value) {
            onNotify(c.getUuid(), value);
        }

        @SuppressWarnings("deprecation")
        @Override public void onCharacteristicChanged(BluetoothGatt g, BluetoothGattCharacteristic c) {
            onNotify(c.getUuid(), c.getValue());
        }
    };
}
