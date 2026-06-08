package com.cleverraven.cataclysmdda;

import android.annotation.TargetApi;
import android.content.Context;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.os.Build;
import android.os.CancellationSignal;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsProvider;
import android.provider.DocumentsContract;
import android.webkit.MimeTypeMap;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

/**
 * DocumentsProvider for Cataclysm: Dark Days Ahead.
 * Exposes the app's external private data directory (/storage/emulated/0/Android/data/package/)
 * so that users can back up saves, mods, and configs without root access.
 * <p>
 * Root document ID is "root" (non-empty) for compatibility with third-party file managers.
 * The provider requires the app process to have been started at least once; after that,
 * file managers can access the data as long as the process is alive.
 */
@TargetApi(Build.VERSION_CODES.KITKAT)
public class CDDADocumentsProvider extends DocumentsProvider {

    // Root document ID - using a non-empty string for better compatibility with various file managers.
    private static final String ROOT_DOCUMENT_ID = "root";
    private File externalDataRoot;

    @Override
    public boolean onCreate() {
        Context context = getContext();
        if (context != null) {
            File externalFilesDir = context.getExternalFilesDir(null);
            if (externalFilesDir != null) {
                externalDataRoot = externalFilesDir.getParentFile();
            }
        }
        return true;
    }

    private void ensureRoot() throws FileNotFoundException {
        if (externalDataRoot == null || !externalDataRoot.exists()) {
            throw new FileNotFoundException("External data root not accessible");
        }
    }

    /**
     * Resolves a document ID to an absolute File object.
     * Document IDs are relative paths under the root, except the root itself which is "root".
     * Security: prevents path traversal by comparing canonical paths.
     */
    private File getFileForDocId(String docId) throws FileNotFoundException {
        ensureRoot();
        File target;
        if (ROOT_DOCUMENT_ID.equals(docId) || docId == null || docId.isEmpty()) {
            target = externalDataRoot;
        } else {
            target = new File(externalDataRoot, docId);
        }
        try {
            String canonicalTarget = target.getCanonicalPath();
            String canonicalRoot = externalDataRoot.getCanonicalPath();
            if (!canonicalTarget.startsWith(canonicalRoot)) {
                throw new FileNotFoundException("Access denied: path escapes root");
            }
        } catch (IOException e) {
            throw new FileNotFoundException("Failed to resolve path: " + e.getMessage());
        }
        if (!target.exists()) {
            throw new FileNotFoundException("File not found: " + docId);
        }
        return target;
    }

    @Override
    public Cursor queryRoots(String[] projection) {
        try {
            ensureRoot();
        } catch (FileNotFoundException e) {
            String[] fallback = (projection != null) ? projection : new String[]{DocumentsContract.Root.COLUMN_ROOT_ID};
            return new MatrixCursor(fallback);
        }

        // Define all possible columns (some may be omitted by the caller's projection)
        String[] allColumns = {
                DocumentsContract.Root.COLUMN_ROOT_ID,
                DocumentsContract.Root.COLUMN_TITLE,
                DocumentsContract.Root.COLUMN_FLAGS,
                DocumentsContract.Root.COLUMN_DOCUMENT_ID,
                DocumentsContract.Root.COLUMN_ICON,
                DocumentsContract.Root.COLUMN_AVAILABLE_BYTES,
                DocumentsContract.Root.COLUMN_CAPACITY_BYTES
        };
        String[] finalProjection = (projection != null) ? projection : allColumns;
        MatrixCursor result = new MatrixCursor(finalProjection);

        Object[] rowValues = new Object[finalProjection.length];
        for (int i = 0; i < finalProjection.length; i++) {
            String col = finalProjection[i];
            if (DocumentsContract.Root.COLUMN_ROOT_ID.equals(col)) {
                rowValues[i] = "external";
            } else if (DocumentsContract.Root.COLUMN_TITLE.equals(col)) {
                rowValues[i] = "CDDA Game Data";
            } else if (DocumentsContract.Root.COLUMN_FLAGS.equals(col)) {
                int flags = DocumentsContract.Root.FLAG_SUPPORTS_CREATE |
                            DocumentsContract.Root.FLAG_LOCAL_ONLY |
                            DocumentsContract.Root.FLAG_SUPPORTS_IS_CHILD;
                rowValues[i] = flags;
            } else if (DocumentsContract.Root.COLUMN_DOCUMENT_ID.equals(col)) {
                rowValues[i] = ROOT_DOCUMENT_ID;
            } else if (DocumentsContract.Root.COLUMN_ICON.equals(col)) {
                rowValues[i] = R.drawable.ic_launcher;
            } else if (DocumentsContract.Root.COLUMN_AVAILABLE_BYTES.equals(col)) {
                // Available on API 23+
                rowValues[i] = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) ? externalDataRoot.getFreeSpace() : 0L;
            } else if (DocumentsContract.Root.COLUMN_CAPACITY_BYTES.equals(col)) {
                rowValues[i] = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) ? externalDataRoot.getTotalSpace() : 0L;
            } else {
                rowValues[i] = null;
            }
        }
        result.addRow(rowValues);
        return result;
    }

    @Override
    public Cursor queryDocument(String documentId, String[] projection) throws FileNotFoundException {
        String[] finalProjection = (projection != null) ? projection : getDocumentProjection();
        MatrixCursor result = new MatrixCursor(finalProjection);
        File file = getFileForDocId(documentId);
        addDocumentRow(result, finalProjection, file, documentId);
        return result;
    }

    @Override
    public Cursor queryChildDocuments(String parentDocumentId, String[] projection, String sortOrder)
            throws FileNotFoundException {
        String[] finalProjection = (projection != null) ? projection : getDocumentProjection();
        MatrixCursor result = new MatrixCursor(finalProjection);
        File parent = getFileForDocId(parentDocumentId);
        if (!parent.isDirectory()) {
            throw new FileNotFoundException("Not a directory: " + parentDocumentId);
        }
        File[] children = parent.listFiles();
        if (children != null) {
            boolean isRoot = ROOT_DOCUMENT_ID.equals(parentDocumentId);
            for (File child : children) {
                String childDocId = isRoot ? child.getName() : parentDocumentId + "/" + child.getName();
                addDocumentRow(result, finalProjection, child, childDocId);
            }
        }
        return result;
    }

    @Override
    public ParcelFileDescriptor openDocument(String documentId, String mode, CancellationSignal signal)
            throws FileNotFoundException {
        File file = getFileForDocId(documentId);
        int modeBits = ParcelFileDescriptor.parseMode(mode);
        return ParcelFileDescriptor.open(file, modeBits);
    }

    @Override
    public String createDocument(String parentDocumentId, String mimeType, String displayName)
            throws FileNotFoundException {
        File parent = getFileForDocId(parentDocumentId);
        if (!parent.isDirectory()) {
            throw new FileNotFoundException("Parent is not a directory: " + parentDocumentId);
        }
        File newFile = new File(parent, displayName);
        try {
            if (DocumentsContract.Document.MIME_TYPE_DIR.equals(mimeType)) {
                if (!newFile.mkdir()) {
                    throw new IOException("Failed to create directory");
                }
            } else {
                if (!newFile.createNewFile()) {
                    throw new IOException("Failed to create file");
                }
            }
        } catch (IOException e) {
            throw new FileNotFoundException("Failed to create document: " + e.getMessage());
        }
        boolean isRootParent = ROOT_DOCUMENT_ID.equals(parentDocumentId);
        return isRootParent ? displayName : parentDocumentId + "/" + displayName;
    }

    @Override
    public void deleteDocument(String documentId) throws FileNotFoundException {
        File file = getFileForDocId(documentId);
        deleteRecursive(file);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            revokeDocumentPermission(documentId);
        }
    }

    private void deleteRecursive(File file) {
        if (file.isDirectory()) {
            File[] children = file.listFiles();
            if (children != null) {
                for (File child : children) {
                    deleteRecursive(child);
                }
            }
        }
        file.delete();
    }

    /**
     * Checks whether a document is a descendant of a parent document.
     * Root is considered the parent of all documents.
     */
    @Override
    public boolean isChildDocument(String parentDocId, String docId) {
        if (parentDocId == null || docId == null) return false;
        if (ROOT_DOCUMENT_ID.equals(parentDocId)) return true;
        if (docId.equals(parentDocId)) return true;
        return docId.startsWith(parentDocId + "/");
    }

    private void addDocumentRow(MatrixCursor cursor, String[] columns, File file, String documentId) {
        Object[] values = new Object[columns.length];
        for (int i = 0; i < columns.length; i++) {
            String col = columns[i];
            if (DocumentsContract.Document.COLUMN_DOCUMENT_ID.equals(col)) {
                values[i] = documentId;
            } else if (DocumentsContract.Document.COLUMN_DISPLAY_NAME.equals(col)) {
                values[i] = file.getName();
            } else if (DocumentsContract.Document.COLUMN_MIME_TYPE.equals(col)) {
                values[i] = file.isDirectory() ? DocumentsContract.Document.MIME_TYPE_DIR : getMimeType(file);
            } else if (DocumentsContract.Document.COLUMN_SIZE.equals(col)) {
                values[i] = file.length();
            } else if (DocumentsContract.Document.COLUMN_LAST_MODIFIED.equals(col)) {
                values[i] = file.lastModified();
            } else if (DocumentsContract.Document.COLUMN_FLAGS.equals(col)) {
                int flags = 0;
                if (file.canWrite()) flags |= DocumentsContract.Document.FLAG_SUPPORTS_WRITE;
                flags |= DocumentsContract.Document.FLAG_SUPPORTS_DELETE;
                if (file.isDirectory() && file.canWrite()) flags |= DocumentsContract.Document.FLAG_DIR_SUPPORTS_CREATE;
                values[i] = flags;
            } else {
                values[i] = null;
            }
        }
        cursor.addRow(values);
    }

    private String getMimeType(File file) {
        String name = file.getName();
        int lastDot = name.lastIndexOf('.');
        if (lastDot > 0) {
            String ext = name.substring(lastDot + 1).toLowerCase();
            String mime = MimeTypeMap.getSingleton().getMimeTypeFromExtension(ext);
            if (mime != null) return mime;
        }
        return "application/octet-stream";
    }

    private static String[] getDocumentProjection() {
        return new String[]{
                DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                DocumentsContract.Document.COLUMN_MIME_TYPE,
                DocumentsContract.Document.COLUMN_SIZE,
                DocumentsContract.Document.COLUMN_LAST_MODIFIED,
                DocumentsContract.Document.COLUMN_FLAGS
        };
    }
}
