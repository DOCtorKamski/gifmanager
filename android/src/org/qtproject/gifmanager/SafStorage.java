package org.qtproject.gifmanager;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.provider.DocumentsContract;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;

/**
 * Small SAF bridge used by AndroidGifStorage.
 *
 * The class deliberately exposes only primitive/String/byte[] values so the
 * C++ side can use straightforward JNI signatures. All methods are safe to
 * call from the loader thread.
 */
public final class SafStorage {
    private SafStorage() {
    }

    private static Uri childDocumentsUri(Uri uri) {
        try {
            final String documentId = DocumentsContract.isTreeUri(uri)
                    ? DocumentsContract.getTreeDocumentId(uri)
                    : DocumentsContract.getDocumentId(uri);
            try {
                return DocumentsContract.buildChildDocumentsUriUsingTree(uri, documentId);
            } catch (Exception ignored) {
                return DocumentsContract.buildChildDocumentsUri(
                        uri.getAuthority(), documentId);
            }
        } catch (Exception ignored) {
            return null;
        }
    }

    private static Uri treeUriForDocument(Uri documentUri) {
        if (DocumentsContract.isTreeUri(documentUri)) {
            return documentUri;
        }

        List<String> segments = documentUri.getPathSegments();
        if (segments.size() >= 2 && "tree".equals(segments.get(0))) {
            return DocumentsContract.buildTreeDocumentUri(
                    documentUri.getAuthority(), segments.get(1));
        }
        return null;
    }

    private static Uri documentUri(Uri tree, String documentId) {
        try {
            return DocumentsContract.buildDocumentUriUsingTree(tree, documentId);
        } catch (Exception ignored) {
            return DocumentsContract.buildDocumentUri(tree.getAuthority(), documentId);
        }
    }

    private static Uri parseUri(String value) {
        try {
            return Uri.parse(value);
        } catch (Exception ignored) {
            return null;
        }
    }

    public static String[] listGifs(Context context, String treeUri) {
        final ContentResolver resolver = context.getContentResolver();
        final Uri tree = parseUri(treeUri);
        final Uri children = tree != null ? childDocumentsUri(tree) : null;
        if (children == null) {
            return null;
        }

        final List<String[]> entries = new ArrayList<>();

        try (Cursor cursor = resolver.query(children, new String[]{
                DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                DocumentsContract.Document.COLUMN_MIME_TYPE
        }, null, null, null)) {
            if (cursor == null) {
                return null;
            }

            final int idIndex = cursor.getColumnIndex(
                    DocumentsContract.Document.COLUMN_DOCUMENT_ID);
            final int nameIndex = cursor.getColumnIndex(
                    DocumentsContract.Document.COLUMN_DISPLAY_NAME);
            final int mimeIndex = cursor.getColumnIndex(
                    DocumentsContract.Document.COLUMN_MIME_TYPE);
            if (idIndex < 0 || nameIndex < 0) {
                return null;
            }

            while (cursor.moveToNext()) {
                final String name = cursor.getString(nameIndex);
                final String mime = mimeIndex >= 0 ? cursor.getString(mimeIndex) : null;
                if (name == null
                        || !name.toLowerCase(Locale.ROOT).endsWith(".gif")
                        || DocumentsContract.Document.MIME_TYPE_DIR.equals(mime)) {
                    continue;
                }

                final String documentId = cursor.getString(idIndex);
                final Uri document = documentUri(tree, documentId);
                entries.add(new String[]{document.toString(), name});
            }
        } catch (Exception e) {
            return null;
        }

        Collections.sort(entries, new Comparator<String[]>() {
            @Override
            public int compare(String[] lhs, String[] rhs) {
                return lhs[1].compareTo(rhs[1]);
            }
        });

        final String[] result = new String[entries.size() * 2];
        for (int i = 0; i < entries.size(); ++i) {
            result[i * 2] = entries.get(i)[0];
            result[i * 2 + 1] = entries.get(i)[1];
        }
        return result;
    }

    public static boolean isReadable(Context context, String treeUri) {
        final ContentResolver resolver = context.getContentResolver();
        final Uri tree = parseUri(treeUri);
        final Uri children = tree != null ? childDocumentsUri(tree) : null;
        if (children == null) {
            return false;
        }

        try (Cursor cursor = resolver.query(children, new String[]{
                DocumentsContract.Document.COLUMN_DOCUMENT_ID
        }, null, null, null)) {
            return cursor != null;
        } catch (Exception ignored) {
            return false;
        }
    }

    public static byte[] readFile(Context context, String documentUri) {
        final ContentResolver resolver = context.getContentResolver();

        try (InputStream input = resolver.openInputStream(Uri.parse(documentUri));
             ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            if (input == null) {
                return null;
            }

            final byte[] buffer = new byte[8192];
            int read;
            while ((read = input.read(buffer)) != -1) {
                output.write(buffer, 0, read);
            }
            return output.toByteArray();
        } catch (Exception ignored) {
            return null;
        }
    }

    /**
     * Decodes the first frame of a GIF at a reduced sample size and returns it
     * as PNG bytes. The grid only needs a small thumbnail, so copying the whole
     * document over JNI and decoding the full-size frame would be wasted work.
     */
    public static byte[] readThumbnail(Context context, String documentUri,
                                       int maxDimension) {
        if (maxDimension <= 0) {
            return null;
        }

        final ContentResolver resolver = context.getContentResolver();
        final Uri uri = parseUri(documentUri);
        if (uri == null) {
            return null;
        }

        final BitmapFactory.Options bounds = new BitmapFactory.Options();
        bounds.inJustDecodeBounds = true;
        try (InputStream input = resolver.openInputStream(uri)) {
            if (input == null) {
                return null;
            }
            BitmapFactory.decodeStream(input, null, bounds);
        } catch (Exception ignored) {
            return null;
        }

        if (bounds.outWidth <= 0 || bounds.outHeight <= 0) {
            return null;
        }

        final BitmapFactory.Options options = new BitmapFactory.Options();
        options.inSampleSize = sampleSizeFor(bounds.outWidth, bounds.outHeight,
                maxDimension);
        options.inPreferredConfig = Bitmap.Config.ARGB_8888;

        Bitmap bitmap = null;
        try (InputStream input = resolver.openInputStream(uri)) {
            if (input == null) {
                return null;
            }
            bitmap = BitmapFactory.decodeStream(input, null, options);
        } catch (Exception ignored) {
            bitmap = null;
        }

        if (bitmap == null) {
            return null;
        }

        try (ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            if (!bitmap.compress(Bitmap.CompressFormat.PNG, 100, output)) {
                return null;
            }
            return output.toByteArray();
        } catch (Exception ignored) {
            return null;
        } finally {
            bitmap.recycle();
        }
    }

    private static int sampleSizeFor(int width, int height, int maxDimension) {
        final int largest = Math.max(width, height);
        int sample = 1;
        while (largest / (sample * 2) >= maxDimension) {
            sample *= 2;
        }
        return sample;
    }

    public static String displayName(Context context, String documentUri) {
        final ContentResolver resolver = context.getContentResolver();

        try (Cursor cursor = resolver.query(Uri.parse(documentUri), new String[]{
                DocumentsContract.Document.COLUMN_DISPLAY_NAME
        }, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                return cursor.getString(0);
            }
        } catch (Exception ignored) {
        }
        return null;
    }

    public static boolean childExists(Context context, String documentUri,
                                      String displayName) {
        final Uri document = parseUri(documentUri);
        final Uri tree = document != null ? treeUriForDocument(document) : null;
        if (tree == null) {
            return false;
        }

        final ContentResolver resolver = context.getContentResolver();
        final Uri children = childDocumentsUri(tree);
        if (children == null) {
            return false;
        }

        try (Cursor cursor = resolver.query(children, new String[]{
                DocumentsContract.Document.COLUMN_DISPLAY_NAME
        }, null, null, null)) {
            if (cursor == null) {
                return false;
            }

            while (cursor.moveToNext()) {
                final String name = cursor.getString(0);
                if (name != null && name.equalsIgnoreCase(displayName)) {
                    return true;
                }
            }
        } catch (Exception ignored) {
        }
        return false;
    }

    public static boolean deleteDocument(Context context, String documentUri) {
        try {
            return DocumentsContract.deleteDocument(
                    context.getContentResolver(), Uri.parse(documentUri));
        } catch (Exception ignored) {
            return false;
        }
    }

    public static boolean renameDocument(Context context, String documentUri,
                                         String displayName) {
        try {
            return DocumentsContract.renameDocument(
                    context.getContentResolver(), Uri.parse(documentUri),
                    displayName) != null;
        } catch (Exception ignored) {
            return false;
        }
    }

    public static boolean takePersistablePermission(Context context, String treeUri) {
        try {
            context.getContentResolver().takePersistableUriPermission(
                    Uri.parse(treeUri),
                    Intent.FLAG_GRANT_READ_URI_PERMISSION
                            | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            return true;
        } catch (Exception ignored) {
            return false;
        }
    }
}
