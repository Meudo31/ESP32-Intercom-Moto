package fr.meudo.intercom;

import android.content.ContentResolver;
import android.net.Uri;

import java.io.IOException;
import java.io.InputStream;
import java.security.MessageDigest;

/** Vérifie un fichier firmware choisi par l'utilisateur et calcule son MD5. */
final class FirmwareFile {

    private FirmwareFile() { }
    static final int ESP_IMAGE_MAGIC = 0xE9;   // premier octet d'une image d'application ESP32

    static String md5AndCheck(ContentResolver cr, Uri uri) throws IOException {
        try (InputStream in = cr.openInputStream(uri)) {
            if (in == null) throw new IOException("Fichier illisible");
            MessageDigest md = MessageDigest.getInstance("MD5");
            byte[] buf = new byte[8192];
            boolean first = true;
            int n;
            while ((n = in.read(buf)) > 0) {
                if (first) {
                    if ((buf[0] & 0xFF) != ESP_IMAGE_MAGIC)
                        throw new IOException("Ce fichier n'est pas un firmware ESP32 (.bin d'application attendu)");
                    first = false;
                }
                md.update(buf, 0, n);
            }
            if (first) throw new IOException("Fichier vide");
            StringBuilder sb = new StringBuilder();
            for (byte b : md.digest()) sb.append(String.format("%02x", b));
            return sb.toString();
        } catch (java.security.NoSuchAlgorithmException e) {
            throw new IOException(e);
        }
    }
}
