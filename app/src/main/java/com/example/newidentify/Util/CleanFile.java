package com.example.newidentify.Util;

import java.io.File;

public class CleanFile {

    public void cleanFile(String filePath) {
        deleteCha(filePath);
        deleteLp4(filePath);
        deleteTxt(filePath);
    }
    public void deleteCha(String filePath) {
        String fileCha = ".cha";

        File folder = new File(filePath);
        File[] files = folder.listFiles();

        if (files != null) {
            for (File file : files) {
                if (file.isFile() && file.getName().endsWith(fileCha)) {
                    if (file.delete()) {
                        System.out.println("Deleted: " + file.getAbsolutePath());
                    } else {
                        System.out.println("Failed to delete: " + file.getAbsolutePath());
                    }
                }
            }
        }
    }
    public void deleteLp4(String filePath) {
        String fileCha = ".lp4";

        File folder = new File(filePath);
        File[] files = folder.listFiles();

        if (files != null) {
            for (File file : files) {
                if (file.isFile() && file.getName().endsWith(fileCha)) {
                    if (file.delete()) {
                        System.out.println("Deleted: " + file.getAbsolutePath());
                    } else {
                        System.out.println("Failed to delete: " + file.getAbsolutePath());
                    }
                }
            }
        }
    }
    public void deleteTxt(String filePath) {
        String fileTxt = ".txt";

        File folder = new File(filePath);
        File[] files = folder.listFiles();

        if (files != null) {
            for (File file : files) {
                if (file.isFile() && file.getName().endsWith(fileTxt)) {
                    if (file.delete()) {
                        System.out.println("Deleted: " + file.getAbsolutePath());
                    } else {
                        System.out.println("Failed to delete: " + file.getAbsolutePath());
                    }
                }
            }
        }
    }
}
