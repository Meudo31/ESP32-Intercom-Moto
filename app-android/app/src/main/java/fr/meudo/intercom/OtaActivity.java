package fr.meudo.intercom;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.os.*;
import android.provider.OpenableColumns;
import android.view.View;
import android.view.WindowManager;
import android.widget.*;
import org.json.JSONObject;
import java.util.concurrent.*;

/** Mise à jour du firmware par Bluetooth (ouvert depuis le tableau de bord). */
public class OtaActivity extends Activity implements BleLink.Listener {

    private static final int REQ_PICK = 1;
    private static final String PREFS = "app";

    private EditText keyEdit;
    private TextView deviceInfo, fileInfo, status;
    private Button btnPick, btnUpload;
    private ProgressBar progress;

    private final ExecutorService worker = Executors.newSingleThreadExecutor();
    private final Handler ui = new Handler(Looper.getMainLooper());
    private BleLink link;

    private Uri fileUri;
    private String fileMd5;
    private long fileSize;
    private boolean uploading = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_ota);
        if (getActionBar() != null) getActionBar().setDisplayHomeAsUpEnabled(true);
        link = BleLink.get(this);

        keyEdit = findViewById(R.id.key);
        deviceInfo = findViewById(R.id.deviceInfo);
        fileInfo = findViewById(R.id.fileInfo);
        status = findViewById(R.id.status);
        btnPick = findViewById(R.id.btnPick);
        btnUpload = findViewById(R.id.btnUpload);
        progress = findViewById(R.id.progress);

        keyEdit.setText(getSharedPreferences(PREFS, MODE_PRIVATE).getString("key", "intercom-moto"));
        btnPick.setOnClickListener(v -> pickFile());
        btnUpload.setOnClickListener(v -> upload());
        setStatus("");
    }

    @Override
    public boolean onNavigateUp() {
        finish();
        return true;
    }

    @Override
    protected void onResume() {
        super.onResume();
        link.addListener(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        link.removeListener(this);
        SharedPreferences.Editor e = getSharedPreferences(PREFS, MODE_PRIVATE).edit();
        e.putString("key", keyEdit.getText().toString()).apply();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        worker.shutdownNow();
    }

    // ---------------------------------------------------------------- liaison

    @Override
    public void onState(int state, String message) {
        if (state == BleLink.CONNECTED) {
            JSONObject j = link.info();
            deviceInfo.setText("Connecté en Bluetooth à " + j.optString("name", "?") + " (" + j.optString("chip", "?") + ")\n"
                    + "Firmware actuel : " + j.optString("fw", "?") + "\n"
                    + "Place pour une mise à jour : " + kb(j.optLong("free_sketch", -1)));
        } else {
            deviceInfo.setText(message.isEmpty() ? "Non connecté à l'intercom." : message);
        }
        refreshButtons();
    }

    @Override public void onData(byte[] packet) { }

    // ---------------------------------------------------------------- fichier

    private void pickFile() {
        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        i.addCategory(Intent.CATEGORY_OPENABLE);
        i.setType("*/*");   // les .bin n'ont pas de type MIME fiable
        startActivityForResult(i, REQ_PICK);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQ_PICK || resultCode != RESULT_OK || data == null || data.getData() == null) return;
        final Uri uri = data.getData();

        String name = "firmware.bin";
        long size = -1;
        try (Cursor c = getContentResolver().query(uri, null, null, null, null)) {
            if (c != null && c.moveToFirst()) {
                int ni = c.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                int si = c.getColumnIndex(OpenableColumns.SIZE);
                if (ni >= 0) name = c.getString(ni);
                if (si >= 0 && !c.isNull(si)) size = c.getLong(si);
            }
        }
        final String fName = name;
        final long fSize = size;

        fileUri = null;
        refreshButtons();
        fileInfo.setText("Vérification de " + fName + "…");
        worker.execute(() -> {
            try {
                if (fSize <= 0) throw new Exception("Taille du fichier inconnue");
                String md5 = FirmwareFile.md5AndCheck(getContentResolver(), uri);
                ui.post(() -> {
                    fileUri = uri;
                    fileSize = fSize;
                    fileMd5 = md5;
                    fileInfo.setText(fName + " — " + kb(fSize) + "\nMD5 " + md5);
                    refreshButtons();
                });
            } catch (Exception e) {
                ui.post(() -> fileInfo.setText(fName + " : " + e.getMessage()));
            }
        });
    }

    // ---------------------------------------------------------------- envoi

    private void upload() {
        final String key = keyEdit.getText().toString();
        if (fileUri == null) return;
        if (key.isEmpty()) { setStatus("Saisissez la clé OTA."); return; }
        long free = link.info().optLong("free_sketch", -1);
        if (free > 0 && fileSize > free) {
            setStatus("Le fichier (" + kb(fileSize) + ") est plus grand que la place disponible sur l'intercom (" + kb(free) + ").");
            return;
        }

        uploading = true;
        refreshButtons();
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        progress.setProgress(0);
        setStatus("Envoi en cours… gardez le téléphone près de l'intercom et l'app ouverte.");
        final Uri uri = fileUri;
        final String md5 = fileMd5;
        final long size = fileSize;

        worker.execute(() -> {
            final long t0 = System.currentTimeMillis();
            try {
                link.otaUpload(getContentResolver(), uri, size, md5, key, (sent, total) -> {
                    final int pct = (int) (sent * 100 / total);
                    final long secs = Math.max(1, (System.currentTimeMillis() - t0) / 1000);
                    final long left = sent > 0 ? (total - sent) * secs / sent : 0;
                    ui.post(() -> {
                        progress.setProgress(pct);
                        setStatus(sent >= total ? "Fichier envoyé, l'intercom vérifie le firmware…"
                                : "Envoi : " + pct + " % (" + kb(sent) + " / " + kb(total) + ") — encore ~" + left + " s");
                    });
                });
                final long secs = (System.currentTimeMillis() - t0) / 1000;
                ui.post(() -> {
                    setStatus("✅ Mise à jour acceptée en " + secs + " s. L'intercom redémarre, l'app se reconnecte toute seule.");
                    done();
                });
            } catch (Exception e) {
                final String m = e.getMessage();
                ui.post(() -> {
                    setStatus("❌ Échec : " + m + "\nL'ancien firmware reste en place.");
                    progress.setProgress(0);
                    done();
                });
            }
        });
    }

    private void done() {
        uploading = false;
        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        refreshButtons();
    }

    // ---------------------------------------------------------------- utilitaires

    private void refreshButtons() {
        boolean connected = link.state() == BleLink.CONNECTED;
        btnPick.setEnabled(!uploading);
        keyEdit.setEnabled(!uploading);
        btnUpload.setEnabled(!uploading && connected && fileUri != null);
    }

    private void setStatus(String s) {
        status.setText(s);
        status.setVisibility(s.isEmpty() ? View.GONE : View.VISIBLE);
    }

    static String kb(long bytes) {
        if (bytes < 0) return "?";
        if (bytes < 1024) return bytes + " o";
        return String.format(java.util.Locale.FRANCE, "%.1f Ko", bytes / 1024.0);
    }
}
