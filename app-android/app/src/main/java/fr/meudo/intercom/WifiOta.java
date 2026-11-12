package fr.meudo.intercom;

import android.content.ContentResolver;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.Uri;
import android.net.wifi.WifiNetworkSpecifier;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Mise à jour par Wi-Fi (Android 10 et plus) : le téléphone rejoint le point d'accès temporaire
 * de l'intercom, sans couper sa connexion Internet, puis envoie le .bin en HTTP.
 * Toutes les méthodes sont bloquantes : à appeler hors du thread UI.
 */
@android.annotation.TargetApi(29)
final class WifiOta implements AutoCloseable {

    static final String URL_UPDATE = "http://192.168.4.1/update";

    private final ConnectivityManager cm;
    private ConnectivityManager.NetworkCallback callback;
    private Network network;

    WifiOta(Context ctx) {
        cm = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
    }

    /** Demande à Android de rejoindre le Wi-Fi de l'intercom (Android affiche une demande à accepter). */
    void connect(String ssid, String password, long timeoutMs) throws IOException, InterruptedException {
        WifiNetworkSpecifier spec = new WifiNetworkSpecifier.Builder()
                .setSsid(ssid)
                .setWpa2Passphrase(password)
                .build();
        NetworkRequest req = new NetworkRequest.Builder()
                .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
                .removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
                .setNetworkSpecifier(spec)
                .build();
        final CountDownLatch latch = new CountDownLatch(1);
        final AtomicReference<Network> found = new AtomicReference<>();
        callback = new ConnectivityManager.NetworkCallback() {
            @Override public void onAvailable(Network n) {
                found.set(n);
                latch.countDown();
            }
            @Override public void onUnavailable() { latch.countDown(); }
        };
        cm.requestNetwork(req, callback);
        if (!latch.await(timeoutMs, TimeUnit.MILLISECONDS) || found.get() == null)
            throw new IOException("impossible de rejoindre le Wi-Fi « " + ssid + " » (demande refusée ou intercom trop loin)");
        network = found.get();
    }

    /** Envoie le firmware ; renvoie quand l'intercom a validé l'image et va redémarrer. */
    void upload(ContentResolver cr, Uri uri, long size, String md5, String key, BleLink.Progress progress)
            throws IOException {
        if (network == null) throw new IOException("Wi-Fi de l'intercom non connecté");
        String boundary = "----IntercomOta" + System.currentTimeMillis();
        byte[] head = ("--" + boundary + "\r\n"
                + "Content-Disposition: form-data; name=\"firmware\"; filename=\"firmware.bin\"\r\n"
                + "Content-Type: application/octet-stream\r\n\r\n").getBytes(StandardCharsets.UTF_8);
        byte[] tail = ("\r\n--" + boundary + "--\r\n").getBytes(StandardCharsets.UTF_8);

        URL url = new URL(URL_UPDATE + "?key=" + URLEncoder.encode(key, "UTF-8") + "&md5=" + md5);
        HttpURLConnection c = (HttpURLConnection) network.openConnection(url);
        try {
            c.setConnectTimeout(10000);
            c.setReadTimeout(60000);   // l'intercom vérifie l'image avant de répondre
            c.setDoOutput(true);
            c.setRequestMethod("POST");
            c.setRequestProperty("Content-Type", "multipart/form-data; boundary=" + boundary);
            c.setFixedLengthStreamingMode(head.length + size + tail.length);

            long sent = 0;
            try (OutputStream out = c.getOutputStream(); InputStream in = cr.openInputStream(uri)) {
                if (in == null) throw new IOException("fichier illisible");
                out.write(head);
                byte[] buf = new byte[4096];
                int n;
                while ((n = in.read(buf)) > 0) {
                    out.write(buf, 0, n);
                    sent += n;
                    progress.onProgress(sent, size);
                }
                out.write(tail);
            }
            if (sent != size) throw new IOException("fichier plus court que prévu");

            int code = c.getResponseCode();
            String body = readAll(code >= 400 ? c.getErrorStream() : c.getInputStream()).trim();
            if (code != 200 || !body.startsWith("DONE"))
                throw new IOException("l'intercom a refusé : " + (body.startsWith("ERR ") ? body.substring(4) : "HTTP " + code + " " + body));
        } finally {
            c.disconnect();
        }
    }

    /** Rend le Wi-Fi : le téléphone revient sur son réseau habituel. */
    @Override
    public void close() {
        if (callback != null) {
            try { cm.unregisterNetworkCallback(callback); } catch (Exception ignored) { }
            callback = null;
        }
        network = null;
    }

    private static String readAll(InputStream in) throws IOException {
        if (in == null) return "";
        try (InputStream i = in) {
            ByteArrayOutputStream b = new ByteArrayOutputStream();
            byte[] buf = new byte[512];
            int n;
            while ((n = i.read(buf)) > 0) b.write(buf, 0, n);
            return b.toString("UTF-8");
        }
    }
}
