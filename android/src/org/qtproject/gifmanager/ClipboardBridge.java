package org.qtproject.gifmanager;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.net.Uri;

/**
 * Bridges Qt clipboard requests to the Android system clipboard. Stateless on
 * purpose: every call receives its Context, so any thread may use it.
 */
public final class ClipboardBridge {

    private ClipboardBridge() {
    }

    /**
     * Copies the document URI to the system clipboard. The clipboard service
     * grants temporary read permission to the app that pastes, so chat apps
     * and file managers can read the GIF directly from the source provider.
     */
    public static boolean copyDocument(Context context, String documentUri,
                                       String label) {
        try {
            final Context app = context.getApplicationContext();
            final ClipboardManager manager = (ClipboardManager) app
                    .getSystemService(Context.CLIPBOARD_SERVICE);
            if (manager == null) {
                return false;
            }
            final ClipData clip = ClipData.newUri(
                    app.getContentResolver(), label, Uri.parse(documentUri));
            manager.setPrimaryClip(clip);
            return true;
        } catch (Exception ignored) {
            return false;
        }
    }
}
