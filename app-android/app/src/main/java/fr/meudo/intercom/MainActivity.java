package fr.meudo.intercom;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.View;
import android.widget.*;
import org.json.JSONObject;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/** Tableau de bord : toutes les infos de l'intercom et toutes ses commandes. */
public class MainActivity extends Activity implements BleLink.Listener {

    private static final int REQ_PERMS = 1, REQ_ENABLE_BT = 2;

    private BleLink link;
    private TextView connState, phone, title, artist, positionText, volLabel, volPhoneLabel, live, info;
    private TextView callState, callNumber, callVolLabel, micGainLabel;
    private Button btnAnswer, btnReject, btnHangup;
    private SeekBar callVolume, micGain;
    private boolean draggingCallVolume, draggingMicGain;
    private int lastCallState = -1;
    private long callEndedAt = 0;   // pour afficher « Appel terminé » quelques secondes
    private long callStartedAt = 0;
    private String callNumberText = "";
    private ProgressBar position;
    private SeekBar volume, volumePhone;
    private EditText name, autoSleep;
    private RadioGroup gain;
    private boolean settingGain;   // mise à jour par le programme : ne pas renvoyer la commande
    private Button btnConnect;
    private final List<View> commandViews = new ArrayList<>();

    private BleProtocol.Data last;
    private long lastMediaCounter = -1;
    private long durationMs = 0;
    private boolean draggingVolume, draggingPhoneVolume;
    // Pour le débit audio
    private long rateBytes = -1, rateTime;
    private double rateKbps;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        link = BleLink.get(this);

        connState = findViewById(R.id.connState);
        phone = findViewById(R.id.phone);
        title = findViewById(R.id.title);
        artist = findViewById(R.id.artist);
        position = findViewById(R.id.position);
        positionText = findViewById(R.id.positionText);
        volLabel = findViewById(R.id.volLabel);
        volPhoneLabel = findViewById(R.id.volPhoneLabel);
        volume = findViewById(R.id.volume);
        volumePhone = findViewById(R.id.volumePhone);
        name = findViewById(R.id.name);
        autoSleep = findViewById(R.id.autoSleep);
        live = findViewById(R.id.live);
        info = findViewById(R.id.info);
        btnConnect = findViewById(R.id.btnConnect);
        gain = findViewById(R.id.gain);
        gain.setOnCheckedChangeListener((g, id) -> {
            if (settingGain || id == -1) return;
            int db = id == R.id.gain6 ? 6 : id == R.id.gain12 ? 12 : 9;
            send("gain=" + db, "Gain de l'ampli : " + db + " dB");
        });
        commandViews.add(findViewById(R.id.gain6));
        commandViews.add(findViewById(R.id.gain9));
        commandViews.add(findViewById(R.id.gain12));

        setupCall();
        btnConnect.setOnClickListener(v -> startConnection());
        command(R.id.btnPrev, "cmd=precedent");
        command(R.id.btnPlay, "cmd=basculer");
        command(R.id.btnStop, "cmd=stop");
        command(R.id.btnNext, "cmd=suivant");
        command(R.id.btnBtDisconnect, "deconnecter=1");
        command(R.id.btnBtReconnect, "reconnecter=1");
        confirmed(R.id.btnBtForget, "Oublier le téléphone ?",
                "L'intercom ne se reconnectera plus tout seul à ce téléphone. Il faudra le réappairer dans le Bluetooth du téléphone.",
                "oublier=1");
        confirmed(R.id.btnSleep, "Mettre en veille profonde ?",
                "La musique et l'app se déconnectent. Pour réveiller l'intercom : appui sur le bouton A (ou coupure de l'alimentation).",
                "veille=1");
        confirmed(R.id.btnRestart, "Redémarrer l'intercom ?", "La musique sera coupée quelques secondes.", "redemarrer=1");

        Button btnName = findViewById(R.id.btnName);
        btnName.setOnClickListener(v -> rename());
        commandViews.add(btnName);
        Button btnAutoSleep = findViewById(R.id.btnAutoSleep);
        btnAutoSleep.setOnClickListener(v -> {
            int m = parseInt(autoSleep.getText().toString(), -1);
            if (m < 0 || m > 240) { toast("Entre 0 et 240 minutes."); return; }
            send("veille_auto=" + m, "Veille automatique : " + (m == 0 ? "jamais" : m + " min"));
        });
        commandViews.add(btnAutoSleep);

        volume.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar s, int p, boolean user) { if (user) volLabel.setText(volText(p)); }
            @Override public void onStartTrackingTouch(SeekBar s) { draggingVolume = true; }
            @Override public void onStopTrackingTouch(SeekBar s) {
                draggingVolume = false;
                send("vol=" + s.getProgress(), null);
            }
        });
        volumePhone.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar s, int p, boolean user) { if (user) volPhoneLabel.setText(phoneVolText(p)); }
            @Override public void onStartTrackingTouch(SeekBar s) { draggingPhoneVolume = true; }
            @Override public void onStopTrackingTouch(SeekBar s) {
                draggingPhoneVolume = false;
                send("voltel=" + s.getProgress(), null);
            }
        });
        commandViews.add(volume);
        commandViews.add(volumePhone);
        commandViews.add(name);
        commandViews.add(autoSleep);

        findViewById(R.id.btnOta).setOnClickListener(v -> startActivity(new Intent(this, OtaActivity.class)));
        clearDashboard();
    }

    @Override
    protected void onResume() {
        super.onResume();
        link.addListener(this);
        startConnection();
    }

    @Override
    protected void onPause() {
        super.onPause();
        link.removeListener(this);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (isFinishing()) link.disconnect();
    }

    // ---------------------------------------------------------------- liaison

    @Override
    public void onState(int state, String message) {
        connState.setText(message.isEmpty() ? "Non connecté à l'intercom." : message);
        boolean connected = state == BleLink.CONNECTED;
        btnConnect.setVisibility(state == BleLink.DISCONNECTED ? View.VISIBLE : View.GONE);
        for (View v : commandViews) v.setEnabled(connected);
        if (connected) {
            JSONObject j = link.info();
            if (!name.hasFocus()) name.setText(j.optString("name", ""));
            showInfo(j);
            lastMediaCounter = -1;
        } else {
            clearDashboard();
        }
    }

    @Override
    public void onData(byte[] packet) {
        BleProtocol.Data d = BleProtocol.Data.parse(packet);
        if (d == null) return;
        last = d;

        // Débit audio, moyenné sur ~2 s
        long now = SystemClock.elapsedRealtime();
        if (rateBytes < 0 || d.audioBytes < rateBytes) {
            rateBytes = d.audioBytes;
            rateTime = now;
        } else if (now - rateTime >= 2000) {
            rateKbps = (d.audioBytes - rateBytes) * 8.0 / (now - rateTime);
            rateBytes = d.audioBytes;
            rateTime = now;
        }

        boolean tel = d.has(BleProtocol.FLAG_TEL);
        boolean playing = d.has(BleProtocol.FLAG_PLAYING);
        if (!draggingVolume) {
            volume.setProgress(d.volume);
            volLabel.setText(volText(d.volume));
        }
        if (!draggingPhoneVolume) {
            volumePhone.setProgress(d.phoneVolume);
            volPhoneLabel.setText(phoneVolText(d.phoneVolume));
        }
        if (!autoSleep.hasFocus()) autoSleep.setText(String.valueOf(d.autoSleepMin));
        int gainId = d.gainDb == 6 ? R.id.gain6 : d.gainDb == 9 ? R.id.gain9 : d.gainDb == 12 ? R.id.gain12 : -1;
        if (gainId != -1 && gain.getCheckedRadioButtonId() != gainId) {
            settingGain = true;
            gain.check(gainId);
            settingGain = false;
        }

        if (durationMs > 0) {
            position.setProgress((int) Math.min(1000, d.positionMs * 1000 / durationMs));
            positionText.setText(time(d.positionMs) + " / " + time(durationMs) + (playing ? "  ▶ lecture" : "  ⏸ pause"));
        } else {
            position.setProgress(0);
            positionText.setText(tel ? (playing ? "▶ lecture" : "⏸ pause") : "");
        }

        String phoneState = tel ? "Connecté" : "Aucun téléphone connecté pour la musique";
        if (tel) {
            phoneState += playing ? " · ▶ lecture" : " · ⏸ pause";
            phoneState += "\nFréquence : " + d.sampleRate + " Hz · débit " + String.format(Locale.FRANCE, "%.0f", rateKbps) + " kbit/s";
            phoneState += "\nSignal : " + signal(d.rssi);
            phoneState += "\nTélécommande (AVRCP) : " + (d.has(BleProtocol.FLAG_AVRCP) ? "oui" : "non");
        }
        phone.setTag(phoneState);
        refreshPhone();

        live.setText("En marche depuis " + duration(d.uptimeS)
                + "\nMémoire libre : " + OtaActivity.kb(d.freeHeap) + " (minimum " + OtaActivity.kb(d.minFreeHeap) + ")"
                + "\nAudio reçu : " + String.format(Locale.FRANCE, "%.1f Mo", d.audioBytes / 1048576.0)
                + "\nBoutons B et C : " + (d.has(BleProtocol.FLAG_BUTTONS) ? "oui" : "non")
                + "\nTéléphone connecté pour les appels : " + (d.has(BleProtocol.FLAG_HFP) ? "oui" : "non")
                + (d.has(BleProtocol.FLAG_OTA) ? "\nMise à jour en cours…" : ""));

        showCall(d);

        if (d.mediaCounter != lastMediaCounter) {
            lastMediaCounter = d.mediaCounter;
            link.readMedia(this::showMedia);
        }
    }

    private String phoneHeader = "";

    private void showMedia(JSONObject m) {
        if (m == null) { lastMediaCounter = -1; return; }
        String n = m.optString("tel_nom", "");
        String a = m.optString("tel_adresse", "");
        phoneHeader = n.isEmpty() && a.isEmpty() ? "" : (n.isEmpty() ? "Téléphone" : n) + (a.isEmpty() ? "" : "  (" + a + ")");
        refreshPhone();

        String t = m.optString("titre", "");
        title.setText(t.isEmpty() ? "—" : t);
        StringBuilder sb = new StringBuilder();
        appendSep(sb, m.optString("artiste", ""));
        appendSep(sb, m.optString("album", ""));
        appendSep(sb, m.optString("genre", ""));
        int piste = m.optInt("piste", 0), nb = m.optInt("nb_pistes", 0);
        if (piste > 0) appendSep(sb, "piste " + piste + (nb > 0 ? "/" + nb : ""));
        artist.setText(sb.toString());
        durationMs = m.optLong("duree_ms", 0);
        callNumberText = m.optString("appel_numero", "");
        if (last != null) showCall(last);
    }

    // ---------------------------------------------------------------- appel

    private void setupCall() {
        callState = findViewById(R.id.callState);
        callNumber = findViewById(R.id.callNumber);
        callVolLabel = findViewById(R.id.callVolLabel);
        micGainLabel = findViewById(R.id.micGainLabel);
        btnAnswer = findViewById(R.id.btnAnswer);
        btnReject = findViewById(R.id.btnReject);
        btnHangup = findViewById(R.id.btnHangup);
        callVolume = findViewById(R.id.callVolume);
        micGain = findViewById(R.id.micGain);
        btnAnswer.setOnClickListener(v -> send("appel=decrocher", null));
        btnReject.setOnClickListener(v -> send("appel=rejeter", null));
        btnHangup.setOnClickListener(v -> send("appel=raccrocher", null));
        callVolume.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar s, int p, boolean user) { if (user) callVolLabel.setText(callVolText(p)); }
            @Override public void onStartTrackingTouch(SeekBar s) { draggingCallVolume = true; }
            @Override public void onStopTrackingTouch(SeekBar s) {
                draggingCallVolume = false;
                send("volappel=" + s.getProgress(), null);
            }
        });
        micGain.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar s, int p, boolean user) { if (user) micGainLabel.setText(micGainText(p + 1)); }
            @Override public void onStartTrackingTouch(SeekBar s) { draggingMicGain = true; }
            @Override public void onStopTrackingTouch(SeekBar s) {
                draggingMicGain = false;
                send("gainmicro=" + (s.getProgress() + 1), "Gain du micro : " + (s.getProgress() + 1));
            }
        });
        commandViews.add(callVolume);
        commandViews.add(micGain);
        showCallButtons(-1);
    }

    private void showCall(BleProtocol.Data d) {
        if (d.callState < 0) {   // firmware plus ancien que 2.1.0
            callState.setText(d.has(BleProtocol.FLAG_RINGING) ? "📞 Appel entrant"
                    : d.has(BleProtocol.FLAG_CALL) ? "📞 Appel en cours"
                    : "Mets à jour le firmware (2.1.0) pour piloter les appels depuis l'app.");
            callNumber.setText("");
            showCallButtons(-1);
            return;
        }
        long now = SystemClock.elapsedRealtime();
        int s = d.callState;
        if (s == BleProtocol.CALL_ACTIVE && lastCallState != BleProtocol.CALL_ACTIVE) callStartedAt = now;
        if (s == BleProtocol.CALL_NONE && lastCallState > BleProtocol.CALL_NONE) callEndedAt = now;
        lastCallState = s;

        String text;
        switch (s) {
            case BleProtocol.CALL_INCOMING: text = "📞 Appel entrant…"; break;
            case BleProtocol.CALL_OUTGOING: text = "📞 Appel sortant, ça sonne…"; break;
            case BleProtocol.CALL_ACTIVE: text = "📞 En communication · " + time(now - callStartedAt); break;
            default:
                if (callEndedAt > 0 && now - callEndedAt < 5000) text = "Appel terminé";
                else text = d.has(BleProtocol.FLAG_HFP) ? "Pas d'appel · prêt pour les appels"
                        : "Le téléphone n'est pas connecté pour les appels";
        }
        callState.setText(text);
        callNumber.setText(s == BleProtocol.CALL_NONE || callNumberText.isEmpty() ? "" : callNumberText);
        showCallButtons(s);

        if (!draggingCallVolume) {
            callVolume.setProgress(d.callVolume);
            callVolLabel.setText(callVolText(d.callVolume));
        }
        if (d.micGain == 0) {
            micGain.setEnabled(false);
            micGainLabel.setText("Micro : absent (AVEC_MICRO = 0 dans config.h)");
        } else if (!draggingMicGain) {
            micGain.setProgress(d.micGain - 1);
            micGainLabel.setText(micGainText(d.micGain));
        }
    }

    private void showCallButtons(int s) {
        boolean connected = link.state() == BleLink.CONNECTED;
        btnAnswer.setEnabled(connected && s == BleProtocol.CALL_INCOMING);
        btnReject.setEnabled(connected && s == BleProtocol.CALL_INCOMING);
        btnHangup.setEnabled(connected && (s == BleProtocol.CALL_ACTIVE || s == BleProtocol.CALL_OUTGOING));
    }

    private static String callVolText(int v) {
        return "Volume de l'appel : " + (v < 0 ? "—" : Math.round(v * 100 / 15.0) + " %");
    }

    private static String micGainText(int g) { return "Gain du micro : " + (g <= 0 ? "—" : "× " + g); }

    private void refreshPhone() {
        Object state = phone.getTag();
        String s = state == null ? "" : state.toString();
        phone.setText(phoneHeader.isEmpty() ? s : phoneHeader + "\n" + s);
    }

    private void showInfo(JSONObject j) {
        info.setText("Nom : " + j.optString("name", "?")
                + "\nFirmware : " + j.optString("fw", "?") + " (compilé le " + j.optString("build", "?") + ")"
                + "\nPuce : " + j.optString("chip", "?") + " rév. " + j.optInt("chip_rev") + ", " + j.optInt("cores") + " cœurs à " + j.optInt("cpu_mhz") + " MHz"
                + "\nFlash : " + j.optLong("flash") / 1048576 + " Mo · programme " + OtaActivity.kb(j.optLong("sketch_size", -1))
                + " · place libre pour une mise à jour " + OtaActivity.kb(j.optLong("free_sketch", -1))
                + "\nMémoire vive : " + OtaActivity.kb(j.optLong("heap_total", -1))
                + "\nAdresse Bluetooth : " + j.optString("mac", "?")
                + "\nDernier démarrage : " + j.optString("reset", "?")
                + "\nSDK : " + j.optString("sdk", "?"));
    }

    private void clearDashboard() {
        last = null;
        phoneHeader = "";
        phone.setTag(null);
        phone.setText("—");
        title.setText("—");
        artist.setText("");
        position.setProgress(0);
        positionText.setText("");
        volLabel.setText(volText(-1));
        volPhoneLabel.setText(phoneVolText(-1));
        live.setText("");
        info.setText("");
        if (callState != null) {
            callState.setText("—");
            callNumber.setText("");
            callVolLabel.setText(callVolText(-1));
            micGainLabel.setText(micGainText(-1));
            callNumberText = "";
            lastCallState = -1;
            callEndedAt = 0;
            showCallButtons(-1);
        }
        rateBytes = -1;
        rateKbps = 0;
        durationMs = 0;
    }

    // ---------------------------------------------------------------- commandes

    private void command(int id, String cmd) {
        View v = findViewById(id);
        v.setOnClickListener(x -> send(cmd, null));
        commandViews.add(v);
    }

    private void confirmed(int id, String question, String detail, String cmd) {
        View v = findViewById(id);
        v.setOnClickListener(x -> new AlertDialog.Builder(this)
                .setTitle(question)
                .setMessage(detail)
                .setPositiveButton("Oui", (dlg, w) -> send(cmd, null))
                .setNegativeButton("Annuler", null)
                .show());
        commandViews.add(v);
    }

    private void rename() {
        String n = name.getText().toString().trim();
        if (n.isEmpty() || n.contains("&") || n.contains("=")) {
            toast("Nom de 1 à 29 caractères, sans & ni =.");
            return;
        }
        new AlertDialog.Builder(this)
                .setTitle("Renommer en « " + n + " » ?")
                .setMessage("L'intercom redémarre. Le téléphone le verra sous le nouveau nom ; s'il ne se reconnecte pas, oublie-le dans le Bluetooth du téléphone et réappaire-le.")
                .setPositiveButton("Renommer", (d, w) -> send("nom=" + n, "Nom changé, redémarrage…"))
                .setNegativeButton("Annuler", null)
                .show();
    }

    private void send(String cmd, String okMessage) {
        link.sendCommand(cmd, err -> {
            if (err != null) toast("❌ " + err);
            else if (okMessage != null) toast(okMessage);
        });
    }

    // ---------------------------------------------------------------- permissions et connexion

    /** Permissions Bluetooth, Bluetooth activé, puis connexion. */
    private void startConnection() {
        List<String> missing = new ArrayList<>();
        if (Build.VERSION.SDK_INT >= 31) {
            if (checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED)
                missing.add(Manifest.permission.BLUETOOTH_SCAN);
            if (checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED)
                missing.add(Manifest.permission.BLUETOOTH_CONNECT);
        } else if (checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            missing.add(Manifest.permission.ACCESS_FINE_LOCATION);   // exigé par Android ≤ 11 pour le scan Bluetooth
        }
        if (!missing.isEmpty()) {
            requestPermissions(missing.toArray(new String[0]), REQ_PERMS);
            return;
        }
        BluetoothAdapter ad = link.adapter();
        if (ad == null) {
            connState.setText("❌ Ce téléphone n'a pas de Bluetooth.");
            return;
        }
        if (!ad.isEnabled()) {
            try {
                startActivityForResult(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE), REQ_ENABLE_BT);
            } catch (SecurityException e) {
                connState.setText("❌ Activez le Bluetooth du téléphone.");
            }
            return;
        }
        link.connect();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
        super.onRequestPermissionsResult(requestCode, permissions, results);
        if (requestCode != REQ_PERMS) return;
        for (int r : results) {
            if (r != PackageManager.PERMISSION_GRANTED) {
                connState.setText("❌ Autorisez « Appareils à proximité » (ou la localisation sur Android 11 et moins) pour trouver l'intercom.");
                return;
            }
        }
        startConnection();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQ_ENABLE_BT && resultCode == RESULT_OK) startConnection();
    }

    // ---------------------------------------------------------------- utilitaires

    private void toast(String s) { Toast.makeText(this, s, Toast.LENGTH_SHORT).show(); }

    private static String volText(int v) { return "Volume de l'intercom : " + (v < 0 ? "—" : v + " %"); }

    private static String phoneVolText(int v) {
        return "Volume du téléphone : " + (v < 0 ? "—" : Math.round(v * 100 / 127.0) + " %");
    }

    private static String signal(int delta) {
        if (delta >= 0) return "bon";
        if (delta > -10) return "moyen (" + delta + " dB)";
        return "faible (" + delta + " dB)";
    }

    private static String time(long ms) {
        long s = ms / 1000;
        return String.format(Locale.FRANCE, "%d:%02d", s / 60, s % 60);
    }

    private static String duration(long s) {
        if (s < 60) return s + " s";
        if (s < 3600) return (s / 60) + " min " + (s % 60) + " s";
        return (s / 3600) + " h " + (s / 60 % 60) + " min";
    }

    private static void appendSep(StringBuilder sb, String s) {
        if (s == null || s.isEmpty()) return;
        if (sb.length() > 0) sb.append(" · ");
        sb.append(s);
    }

    private static int parseInt(String s, int def) {
        try { return Integer.parseInt(s.trim()); } catch (NumberFormatException e) { return def; }
    }
}
