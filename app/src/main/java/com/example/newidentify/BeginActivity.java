package com.example.newidentify;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.PersistableBundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class BeginActivity extends AppCompatActivity {
    Button btn_login, btn_signUp,btn_clear_lp4;
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.begin_page);

        btn_login = findViewById(R.id.btn_login);
        btn_signUp = findViewById(R.id.btn_signUp);
        btn_clear_lp4 = findViewById(R.id.btn_clear_lp4);
        initPermission();
        Intent it = new Intent();

        btn_login.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                it.setClass(BeginActivity.this, LoginActivity.class);
                startActivity(it);
            }
        });

        btn_signUp.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                it.setClass(BeginActivity.this, MainActivity.class);
                startActivity(it);
            }
        });

        btn_clear_lp4.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                deleteLp4();
                deleteCha();
                deleteTxt();
            }
        });
    }

    /**
     * 檢查權限
     **/
    private void initPermission() {
        List<String> mPermissionList = new ArrayList<>();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            mPermissionList.add(Manifest.permission.BLUETOOTH_SCAN);
            mPermissionList.add(Manifest.permission.BLUETOOTH_ADVERTISE);
            mPermissionList.add(Manifest.permission.BLUETOOTH_CONNECT);
        }
        mPermissionList.add(Manifest.permission.ACCESS_COARSE_LOCATION);
        mPermissionList.add(Manifest.permission.ACCESS_FINE_LOCATION);
        mPermissionList.add(Manifest.permission.WRITE_EXTERNAL_STORAGE);
        mPermissionList.add(Manifest.permission.READ_EXTERNAL_STORAGE);

        ActivityCompat.requestPermissions(this, mPermissionList.toArray(new String[0]), 1001);
    }

    private void deleteLp4() {
        String folderName = "Apple_ID_Detect";
        String fileLp4 = ".lp4";

        File folder = new File(Environment.getExternalStorageDirectory().getAbsolutePath(), folderName);
        File[] files = folder.listFiles();

        if (files != null) {
            for (File file : files) {
                if (file.isFile() && file.getName().endsWith(fileLp4)) {
                    if (file.delete()) {
                        System.out.println("Deleted: " + file.getAbsolutePath());
                    } else {
                        System.out.println("Failed to delete: " + file.getAbsolutePath());
                    }
                }
            }
        }
        Toast.makeText(this, "已清除量測檔案", Toast.LENGTH_SHORT).show();
    }
    private void deleteCha() {
        String folderName = "Apple_ID_Detect";
        String fileCha = ".cha";

        File folder = new File(Environment.getExternalStorageDirectory().getAbsolutePath(), folderName);
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
    private void deleteTxt() {
        String folderName = "Apple_ID_Detect";
        String fileTxt = ".txt";

        File folder = new File(Environment.getExternalStorageDirectory().getAbsolutePath(), folderName);
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
