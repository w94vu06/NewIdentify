package com.example.newidentify;

import static java.lang.Math.abs;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.obsez.android.lib.filechooser.ChooserDialog;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class MainActivity extends AppCompatActivity {

    /** UI **/
    Button btn_choose,btn_identify;
    TextView txt_file,txt_result;
    /** Parameter **/
    private String filePath,fileName,ans = "";
    private int y,count;
    private Boolean x;
    private ChooserDialog chooserDialog; //檔案選擇器
    private Map<String, String> dataMap = new HashMap<>();
    private ArrayList<Double> heartRate = new ArrayList<>();
    private ArrayList<Double> PI = new ArrayList<>();
    private ArrayList<Double> CVI = new ArrayList<>();
    private ArrayList<Double> C1a = new ArrayList<>();
    double averageHR, averagePI, averageCVI, averageC1a;
    double maxPI, maxCVI, maxC1a;
    double minPI, minCVI, minC1a;

    identifyPlan identifyPlan = new identifyPlan();


    // Used to load the 'newidentify' library on application startup.
    static {
        System.loadLibrary("newidentify");
        System.loadLibrary("lp4tocha");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        initParameter();
        initPermission();
    }

    private void initParameter(){
        btn_choose = findViewById(R.id.btn_choose);
        btn_identify = findViewById(R.id.btn_identify);
        txt_file = findViewById(R.id.txt_file);
        txt_result = findViewById(R.id.txt_result);
        y = 5; //設定檔案收集數量
        try {
            File file = new File(filePath);
            if (file.mkdir()) {
                System.out.println("新增資料夾");
            }else {
                System.out.println("資料夾已存在");
            }
        }catch (Exception e){
            Log.e("where", e.toString());
        }
    }

    /** 檢查權限 **/
    public void initPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
                && checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{android.Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1000);
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
                && checkSelfPermission(android.Manifest.permission.READ_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 1);
        }
        initChooser();
    }

    /** 檔案選擇器 **/
    public void initChooser() {
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                chooserDialog = new ChooserDialog(MainActivity.this)
                        .withStartFile("/storage/emulated/0/EcgFiles/")
                        .withOnCancelListener(new DialogInterface.OnCancelListener() {
                            @Override
                            public void onCancel(DialogInterface dialogInterface) {
                                dialogInterface.cancel();
                            }
                        })
                        .withChosenListener(new ChooserDialog.Result() {
                            @Override
                            public void onChoosePath(String dir, File dirFile) {
                                filePath = dir;
                                txt_file.setText(filePath);
                                File file = new File(dir);
                                fileName = file.getName();
                                filePath = file.getParent();
                                Log.d("fileName",fileName);
                                Log.d("filePath",filePath);
                            }
                        })
                        .withOnBackPressedListener(dialog -> chooserDialog.goBack())
                        .withOnLastBackPressedListener(dialog -> dialog.cancel());
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        btn_choose.setOnClickListener(new View.OnClickListener() {
                            @Override
                            public void onClick(View v) {
                                chooserDialog.build();
                                chooserDialog.show();
                            }
                        });
                    }
                });
            }
        });
        thread.start();
        initCheck();
    }

    /** ID識別按鈕事件 **/
    private void initCheck(){
        btn_identify.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (fileName != null){
                    if (fileName.endsWith(".lp4")){
                        decpEcgFile(filePath);
                        int y = fileName.length();
                        String j = fileName.substring(0,y-4);
                        fileName = j + ".cha";
                        initIdentify();
                    } else if (fileName.endsWith(".cha") | fileName.endsWith(".CHA")) {
                        initIdentify();
                    } else {
                        txt_result.setText("不支援此檔案類型");
                    }
                }else {
                    txt_result.setText("尚未選擇檔案");
                }
            }
        });
    }

    /** 執行c++檔 **/
    private void initIdentify(){
        int x = stringFromJNI(fileName,filePath);
        if (x == 1){
            txt_result.setText("檔案訊號error，請換個檔案繼續");
        }else {
            count +=1;
        }
        Log.d("getValue",String.valueOf(x));
        filePath = "/storage/emulated/0/ECGFiles/test/";
        fileName = fileName.substring(0,fileName.length()-4);
        try {
            File file = new File(filePath+"/r_"+fileName+".txt");
            if (file.isFile() && file.exists()) {
                BufferedReader reader = new BufferedReader(new FileReader(file));
                String line;
                while ((line = reader.readLine()) != null) {
                    txt_result.setText(line);
                }
                line = txt_result.getText().toString();
                String[] parts = line.split(",");

                for (String part : parts) {
                    String[] nameValue = part.split(":");
                    if (nameValue.length == 2) {
                        String name = nameValue[0];
                        String value =nameValue[1];
                        dataMap.put(name, value);
                    }
                }
                Log.d("getMap", dataMap.toString());
                if (heartRate.size() < y){
                    heartRate.add(Double.parseDouble(dataMap.get("Average")));
                    PI.add(Double.parseDouble(dataMap.get("PI")));
                    CVI.add(Double.parseDouble(dataMap.get("CVI")));
                    C1a.add(Double.parseDouble(dataMap.get("C1a")));
                    getValue();
                }else if (heartRate.size() == y){
                    judgeValue();
                }
                Log.d("getListSize",String.valueOf(heartRate.size()));
                Log.d("ListValue",heartRate.toString());
                String s = String.format("%s \nHR: %s \nPI: %s \nCVI: %s \nC1a: %s \n輸入檔案數量: %d",ans,heartRate.toString(),PI.toString(),CVI.toString(),C1a.toString(),count);
                txt_result.setText(s);
                reader.close();
            }
        } catch (Exception e){
            Log.e("catchError", e.toString());
        }
    };

    private void getValue(){
        averageHR = heartRate.stream().mapToDouble(Double::valueOf).average().getAsDouble();
        averagePI = PI.stream().mapToDouble(Double::valueOf).average().getAsDouble();
        maxPI = PI.stream().mapToDouble(Double::valueOf).max().getAsDouble();
        minPI = PI.stream().mapToDouble(Double::valueOf).min().getAsDouble();
        averageCVI = CVI.stream().mapToDouble(Double::valueOf).average().getAsDouble();
        maxCVI = CVI.stream().mapToDouble(Double::valueOf).max().getAsDouble();
        minCVI = CVI.stream().mapToDouble(Double::valueOf).min().getAsDouble();
        averageC1a = C1a.stream().mapToDouble(Double::valueOf).average().getAsDouble();
        maxC1a = C1a.stream().mapToDouble(Double::valueOf).max().getAsDouble();
        minC1a = C1a.stream().mapToDouble(Double::valueOf).min().getAsDouble();
    }

    private void judgeValue(){
        for (Double value : heartRate){
            if (value > 100 || value > averageHR*1.2){
                x = true;
            }else {
                x = false;
            }
        }
        if (x == true || maxPI-minPI > 0.25 || maxCVI-minCVI > 4.5 || abs(maxC1a-averageC1a) >= 20 || abs(minC1a-averageC1a) >= 20){
            ans = identifyPlan.Second(averageHR, averagePI, averageCVI, averageC1a);
        }else {
            ans = identifyPlan.First(averageHR, averagePI, averageCVI, averageC1a);
        }
        try {
            if (ans.equals("本人")){
                Log.d("ListValue",dataMap.get("Average"));
                heartRate.set(count%y-1,Double.parseDouble(dataMap.get("Average")));
                PI.set(count%y-1,Double.parseDouble(dataMap.get("PI")));
                CVI.set(count%y-1,Double.parseDouble(dataMap.get("CVI")));
                C1a.set(count%y-1,Double.parseDouble(dataMap.get("C1a")));
            }else{
                newDialog();
            }
        }catch (Exception e){
            Log.e("super",e.toString());
        }
    }

    private void newDialog(){
        AlertDialog.Builder alertDialog = new AlertDialog.Builder(MainActivity.this);
        alertDialog.setTitle("是否為帳號本人使用？");
        alertDialog.setPositiveButton("是",((dialog, which) -> {}));
        alertDialog.setNegativeButton("否",((dialog, which) -> {}));
        AlertDialog dialog = alertDialog.create();
        dialog.show();
        dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener((v -> {
            switch (y){
                case 5:
                    y = 10;
                    break;
                case 10:
                    y = 15;
                    break;
                case 15:
                    y = 20;
                    break;
            }
            dialog.dismiss();
        }));
        dialog.getButton(AlertDialog.BUTTON_NEGATIVE).setOnClickListener((v ->{
            txt_result.setText("非本人");
            dialog.dismiss();
        }));

        dialog.setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);
    }

    public native int stringFromJNI(String name, String path);

    public native int decpEcgFile(String path);
}