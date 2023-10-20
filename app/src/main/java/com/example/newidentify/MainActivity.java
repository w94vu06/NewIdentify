package com.example.newidentify;

import static java.lang.Math.abs;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.media.MediaScannerConnection;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Base64;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.obsez.android.lib.filechooser.ChooserDialog;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

public class MainActivity extends AppCompatActivity {
    private SharedPreferences preferences;
    private SharedPreferences.Editor editor;
    /** UI **/
    Button btn_choose,btn_detect,btn_stop;
    TextView txt_file,txt_result,txt_value,txt_count;
    /** Parameter **/
    private String path,filePath,fileName,ans;
    private int dataCollectionLimit,count;
    private Boolean checkX;
    private ChooserDialog chooserDialog; //檔案選擇器
    private Map<String, String> dataMap = new HashMap<>();
    private ArrayList<Double> heartRate = new ArrayList<>();
    private ArrayList<Double> PI = new ArrayList<>();
    private ArrayList<Double> CVI = new ArrayList<>();
    private ArrayList<Double> C1a = new ArrayList<>();
    double averageHR, averagePI, averageCVI, averageC1a;
    double maxPI, maxCVI, maxC1a;
    double minPI, minCVI, minC1a;
    double ValueHR, ValuePI, ValueCvi, ValueC1a;

    identifyPlan identifyPlan = new identifyPlan();

    boolean fromLogin = false;

    // Used to load the 'newidentify' library on application startup.
    static {
        System.loadLibrary("newidentify");
        System.loadLibrary("lp4tocha");
    }
    /** BLE */
    public static Activity global_activity;
    public static TextView txt_countDown;

    static BT4 bt4 = new BT4();
    public static TextView BT_Status_Text;
    public static TextView BT_Power_Text;
//    public static TextView Percent_Text;
    public static LineChart chart1;
    //////畫心電圖使用///////
    public static ArrayList<Entry> chartSet1Entries = new  ArrayList<Entry>();
    public static ArrayList<Double> oldValue = new  ArrayList<Double>();
    public static LineDataSet chartSet1 = new LineDataSet(null,"");
    static double[] mX = {0.0,0.0,0.0,0.0,0.0};
    static double[] mY = {0.0,0.0,0.0,0.0,0.0};
    static int[] mStreamBuf = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
    static double[] mAcoef = { 0.00001347408952448771,
            0.00005389635809795083,
            0.00008084453714692624,
            0.00005389635809795083,
            0.00001347408952448771 };
    static double[] mBcoef = {1.00000000000000000000,
            -3.67172908916193470000,
            5.06799838673418980000,
            -3.11596692520174570000,
            0.71991032729187143000 };
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        preferences = getSharedPreferences("my_preferences", MODE_PRIVATE);
        editor = preferences.edit();
        BT_Status_Text = findViewById(R.id.textView);
        BT_Power_Text = findViewById(R.id.textView4);
//        Percent_Text = findViewById(R.id.textView7);

        global_activity = this;
        chart1 = findViewById(R.id.linechart);
        initchart();
        initParameter();
        initPermission();
        bt4.Bluetooth_init();
    }

    private void initParameter(){
        btn_detect = findViewById(R.id.btn_detect);
        btn_stop = findViewById(R.id.btn_stop);
        btn_choose = findViewById(R.id.btn_choose);
//        btn_identify = findViewById(R.id.btn_identify);
        txt_file = findViewById(R.id.txt_file);
        txt_result = findViewById(R.id.txt_result);
        txt_value = findViewById(R.id.txt_value);
        txt_count = findViewById(R.id.txt_count);
        txt_countDown = findViewById(R.id.txt_countDown);

        dataCollectionLimit = 4; //設定檔案收集數量，最高20
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
        if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{android.Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1000);
        }
        if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 1);
        }
        initChooser();
    }

    /** 檔案選擇器 **/
    public void initChooser() {
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                //找外部儲存
                String externalStorageDirectory = Environment.getExternalStorageDirectory().getAbsolutePath();
                chooserDialog = new ChooserDialog(MainActivity.this)
//                        .withStartFile("/storage/emulated/0/")
                        .withStartFile(externalStorageDirectory)
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
                                File file = new File(dir);
                                fileName = file.getName();
                                path = filePath.substring(0,filePath.length()-fileName.length());
                                txt_file.setText(fileName);
                                initCheck();
                                if (fromLogin) {
                                    judgeValue();
                                }
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
    }

    /** ID識別按鈕事件 **/
    private void initCheck(){
        if (fileName != null){
            if (fileName.endsWith(".lp4")){
                decpEcgFile(filePath);
                int u = fileName.length();
                String j = fileName.substring(0,u-4);
                fileName = j + ".cha";
                if (!fromLogin) {
                    initIdentify();
                }
            } else if (fileName.endsWith(".cha") | fileName.endsWith(".CHA")) {
                if (!fromLogin) {
                    initIdentify();
                }
            } else {
                txt_result.setText("不支援此檔案類型");
            }
        }else {
            txt_result.setText("尚未選擇檔案");
        }
    }
    /** ID識別按鈕事件 **/
    private void initCheck2(String fileName,String filePath){
        if (fileName != null){
            if (fileName.endsWith(".lp4")){
                decpEcgFile(filePath);
                int u = fileName.length();
                String j = fileName.substring(0,u-4);
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

    /** 執行c++檔 **/
    private void initIdentify(){
        int x = anaEcgFile(fileName, path);
        if (x == 1){
            txt_result.setText("檔案訊號error，請換個檔案繼續");
        }else {
            count +=1;
        }
        filePath = path;
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
                ValueHR = Double.parseDouble(dataMap.get("Average"));
                ValuePI = Double.parseDouble(dataMap.get("PI"));
                ValueCvi = Double.parseDouble(dataMap.get("CVI"));
                ValueC1a = Double.parseDouble(dataMap.get("C1a"));
                if (heartRate.size() < dataCollectionLimit){//如果數據數小於dataCollectionLimit就繼續收集
                    heartRate.add(ValueHR);//把LP4算好的結果加進List
                    PI.add(ValuePI);
                    CVI.add(ValueCvi);
                    C1a.add(ValueC1a);
                    getValue();
                }else if (heartRate.size()  == dataCollectionLimit){
                    if (!fromLogin) {
                        ShowToast("註冊成功...");
                        saveInfoToPreference();
                        signUpDialog();
                    }
                }
                String s = String.format("HR: %s \n PI: %s \n CVI: %s \n C1a: %s",heartRate.toString(),PI.toString(),CVI.toString(),C1a.toString());
                Log.d("getListSize",String.valueOf(heartRate.size()));
                Log.d("ListValue",s);
                if (ans != null){
                    txt_result.setText(ans);
                }else{
                    txt_result.setText("檔案數量不足");
                }
                txt_value.setText(s);
                txt_count.setText(String.format("目前設定的檔案數量: %d\n輸入檔案數量: %d", dataCollectionLimit + 1,count));
                reader.close();
            }
        } catch (Exception e){
            Log.e("catchError", e.toString());
        }
    };

    private void getValue(){//算各項數值的平均值
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
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

    }
    //把上面從C拿到的各項數據存Preference，之後移植judgeValue到登入
    private void judgeValue(){
        for (Double value : heartRate){
            if (value > 100 || value > averageHR*1.2){
                checkX = true;
                Log.d("HRValue",String.format("x: %b\nhr: %f\naverageHR: %f\naverageHR*1.2: %f", checkX,value,averageHR,averageHR*1.2));
                break;
            }else {
                checkX = false;
                Log.d("HRValue",String.format("averageHR: %f\naverageHR*1.2: %f",averageHR,averageHR*1.2));
            }
        }
        if (checkX || maxPI-minPI > 0.25 || maxCVI-minCVI > 4.5 || abs(maxC1a-averageC1a) >= 20 || abs(minC1a-averageC1a) >= 20){
            ans = identifyPlan.Second(averageHR, ValueHR, averagePI, ValuePI, averageCVI, ValueCvi, averageC1a, ValueC1a);
        }else {
            ans = identifyPlan.First(averageHR, ValueHR, averagePI, ValuePI, averageCVI, ValueCvi, averageC1a, ValueC1a);
        }
        try {
            if (ans.equals("本人")){
                Log.d("ListValue",dataMap.get("Average"));
                heartRate.set(count% dataCollectionLimit -1,ValueHR);
                PI.set(count% dataCollectionLimit -1,ValuePI);
                CVI.set(count% dataCollectionLimit -1,ValueCvi);
                C1a.set(count% dataCollectionLimit -1,ValueC1a);
            }else{
                if (fromLogin) {
                    newDialog();
                }
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
            switch (dataCollectionLimit){
                case 5:
                    dataCollectionLimit = 10;
                    break;
                case 10:
                    dataCollectionLimit = 15;
                    break;
                case 15:
                    dataCollectionLimit = 20;
                    break;
            }
            heartRate.add(ValueHR);
            PI.add(ValuePI);
            CVI.add(ValueCvi);
            C1a.add(ValueC1a);
            String s = String.format("HR: %s\nPI: %s\nCVI: %s\nC1a: %s",heartRate.toString(),PI.toString(),CVI.toString(),C1a.toString());
            if (!fromLogin) {
                txt_count.setText(String.format("目前設定的檔案數量: %d\n輸入檔案數量: %d", dataCollectionLimit, count));
                txt_result.setText("檔案數量不足");
                txt_value.setText(s);
            } else {
                ShowToast("請再次量測");
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

    public void saveInfoToPreference() {
        // 儲存Map
        for (Map.Entry<String, String> entry : dataMap.entrySet()) {
            editor.putString(entry.getKey(), entry.getValue());
        }

        // 儲存ArrayList
        saveArrayList(editor, "heartRate", heartRate);
        saveArrayList(editor, "PI", PI);
        saveArrayList(editor, "CVI", CVI);
        saveArrayList(editor, "C1a", C1a);

        // 儲存其他變數
        editor.putFloat("averageHR", (float) averageHR);
        editor.putFloat("averagePI", (float) averagePI);
        editor.putFloat("averageCVI", (float) averageCVI);
        editor.putFloat("averageC1a", (float) averageC1a);
        editor.putFloat("maxPI", (float) maxPI);
        editor.putFloat("maxCVI", (float) maxCVI);
        editor.putFloat("maxC1a", (float) maxC1a);
        editor.putFloat("minPI", (float) minPI);
        editor.putFloat("minCVI", (float) minCVI);
        editor.putFloat("minC1a", (float) minC1a);
        editor.putFloat("ValueHR", (float) ValueHR);
        editor.putFloat("ValuePI", (float) ValuePI);
        editor.putFloat("ValueCvi", (float) ValueCvi);
        editor.putFloat("ValueC1a", (float) ValueC1a);

        editor.apply();
    }

    // 儲存ArrayList到SharedPreferences
    private static void saveArrayList(SharedPreferences.Editor editor, String key, ArrayList<Double> list) {
        // 先將ArrayList轉換成Set
        Set<String> set = new HashSet<>();
        for (Double value : list) {
            set.add(String.valueOf(value));
        }
        editor.putStringSet(key, set);
    }

    private void signUpDialog(){
        AlertDialog.Builder alertDialog = new AlertDialog.Builder(MainActivity.this);
        alertDialog.setTitle("是否回到登入頁面？");
        alertDialog.setPositiveButton("是", (dialog, which) -> {
            // 跳轉到 MainActivity
            dialog.dismiss();
            fromLogin = true;
            txt_result.setText("");
            txt_count.setText("");
            txt_value.setText("");
//            Intent intent = new Intent(MainActivity.this, LoginActivity.class);
//            startActivity(intent);
//            finish();
        });
        alertDialog.setNegativeButton("否", (dialog, which) -> {
            dialog.dismiss();
        });
        AlertDialog dialog = alertDialog.create();
        dialog.show();

        dialog.setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);
    }

    public static void DrawChart(byte[] result) {
        try {
            global_activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    String receive = "";
                    for (int i = 0; i < result.length; i++)
                        receive += "  " + Integer.toString((result[i] & 0xff) + 0x100, 16).substring(1);
                    double ch2 = 0;
                    ch2 = bt4.byteArrayToInt(new byte[]{result[4]}) * 128 + bt4.byteArrayToInt(new byte[]{result[5]});
                    ch2 = ch2 * 1.7;
                    int ch4 = getStreamLP((int) ch2);
                    if (ch4 >= 2500) {
                        ch4 = 2500;
                    } else if (ch4 <= 1500) {
                        ch4 = 1500;
                    }
                    if (chartSet1Entries.size() >= 300) {
                        ArrayList<Entry> temp = new ArrayList<Entry>();
                        ArrayList<Double> temp_old = new ArrayList<Double>();
                        for (int i = 1; i < chartSet1Entries.size(); i++) {
                            Entry chartSet1Entrie = new Entry(temp.size(), chartSet1Entries.get(i).getY());
                            temp.add(chartSet1Entrie);
                            temp_old.add(oldValue.get(i));
                        }
                        chartSet1Entries = temp;
                        oldValue = temp_old;
                    }
                    oldValue.add((double) ch4);

                    double nvalue = (oldValue.get(oldValue.size() - 1));

                    if (oldValue.size() > 1) {
                        nvalue = Butterworth(oldValue);
                    }
                    Entry chartSet1Entrie = new Entry(chartSet1Entries.size(), (float) nvalue);
                    chartSet1Entries.add(chartSet1Entrie);
                    chartSet1.setValues(chartSet1Entries);
                    chart1.setData(new LineData(chartSet1));
                    chart1.setVisibleXRangeMinimum(300);
                    chart1.invalidate();
                }
            });
        } catch (Exception ex) {
            Log.d("wwwww", "eeeeeerrr = " + ex.toString());
        }
    }

    public static int getStreamLP(int NewSample) {
        int tmp = 0;
        for (int array = 4; array >= 1; array--) {
            mX[array] = mX[array - 1];
            mY[array] = mY[array - 1];
        }
        mX[0] = (double) (NewSample);
        mY[0] = mAcoef[0] * mX[0];
        for (int i = 1; i <= 4; i++) {
            mY[0] += mAcoef[i] * mX[i] - mBcoef[i] * mY[i];
        }
        for (int array = 20; array >= 1; array--) {
            mStreamBuf[array] = mStreamBuf[array - 1];
        }
        mStreamBuf[0] = NewSample;
        tmp = mStreamBuf[20] + (2000 - (int) (mY[0]));
        return tmp;
    }

    public static double Butterworth(ArrayList<Double> indata) {
        try {
            double deltaTimeinsec = 0.000125;
            double CutOff = 1921;
            double Samplingrate = 1 / deltaTimeinsec;
            Samplingrate = 10000;
            int dF2 = indata.size() - 1;        // The data range is set with dF2
            double[] Dat2 = new double[dF2 + 4]; // Array with 4 extra points front and back
            ArrayList<Double> data = new ArrayList<Double>(); // Ptr., changes passed data
            // Copy indata to Dat2
            for (int r = 0; r < dF2; r++) {
                Dat2[2 + r] = indata.get(r);
            }
            Dat2[1] = Dat2[0] = indata.get(0);
            Dat2[dF2 + 3] = Dat2[dF2 + 2] = indata.get(dF2);
            double pi = 3.14159265358979;
            double wc = Math.tan(CutOff * pi / Samplingrate);
            double k1 = 1.414213562 * wc; // Sqrt(2) * wc
            double k2 = wc * wc;
            double a = k2 / (1 + k1 + k2);
            double b = 2 * a;
            double c = a;
            double k3 = b / k2;
            double d = -2 * a + k3;
            double e = 1 - (2 * a) - k3;
            // RECURSIVE TRIGGERS - ENABLE filter is performed (first, last points constant)
            double[] DatYt = new double[dF2 + 4];
            DatYt[1] = DatYt[0] = indata.get(0);
            for (int s = 2; s < dF2 + 2; s++) {
                DatYt[s] = a * Dat2[s] + b * Dat2[s - 1] + c * Dat2[s - 2]
                        + d * DatYt[s - 1] + e * DatYt[s - 2];
            }
            DatYt[dF2 + 3] = DatYt[dF2 + 2] = DatYt[dF2 + 1];

            // FORWARD filter
            double[] DatZt = new double[dF2 + 2];
            DatZt[dF2] = DatYt[dF2 + 2];
            DatZt[dF2 + 1] = DatYt[dF2 + 3];
            for (int t = -dF2 + 1; t <= 0; t++) {
                DatZt[-t] = a * DatYt[-t + 2] + b * DatYt[-t + 3] + c * DatYt[-t + 4]
                        + d * DatZt[-t + 1] + e * DatZt[-t + 2];
            }

            // Calculated points copied for return
            for (int p = 0; p < dF2; p++) {
                data.add(DatZt[p]);
            }


            return data.get(data.size() - 1);
        } catch (Exception ex) {
            Log.d("xxxxx", "exexexex = " + ex.toString());
            return indata.get(indata.size() - 1);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == 350) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Log.d("wwwww", "搜尋設備");
            } else {
                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.ACCESS_FINE_LOCATION},
                        350
                );

            }
        }
    }
    public static void ShowToast(final String message) {
        global_activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(global_activity, message, Toast.LENGTH_LONG).show();
            }
        });
    }//ShowToast

    public void InitAction(View view) {
        bt4.Bluetooth_init();
    }
    @SuppressLint("HandlerLeak")
    public void PowerAction(View view) {
        bt4.WhenConnectBLE(new Handler() {
            @Override
            public void handleMessage(Message msg2) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        BT_Power_Text.setText(bt4.Battery_Percent + "%");
                    }
                });
            }
        });
    }

    @SuppressLint("HandlerLeak")
    public void StopAction(View view) {
        ShowToast("正在停止跑波...");
        bt4.StopMeasure(new Handler() {
            @Override
            public void handleMessage(Message msg2) {
                ShowToast("完成停止跑波");
            }
        });
//        txt_result.setText("");
//        txt_count.setText("");
//        txt_value.setText("");
    }
    @SuppressLint("HandlerLeak")
    public void RecordWaveAction(View view) {
        bt4.Bluetooth_init();

        bt4.Wave(true, new Handler());
    }
    @SuppressLint("HandlerLeak")
    public void ReadFileAction(View view) {

        final int[] step = {0};
//        Percent_Text.setText("0 %");
        bt4.Record_Size(new Handler() {
            @Override
            public void handleMessage(Message msg) {

                if (step[0] == 0) {
                    Log.d("wwwww", "讀取檔案大小");

                    bt4.File_Size(this);
                }

                if (step[0] == 1) {
                    Log.d("wwwww", "打開檔案");

                    bt4.Open_File(this);
                }

                if (step[0] == 2) {
                    Log.d("wwwww", "讀取檔案");
                    bt4.ReadData(this);
                    saveLP4(bt4.file_data);
                }

//                if (step[0] == 3) {
//                    String EcgFileBase64 = encodeFileToBase64Binary(bt4.file_data);
//                    ShowToast("ECG Base64 = " + EcgFileBase64);
//                }
//
//                if (step[0] == 4) {
//                    Log.d("wwwww", "刪除所有檔案");
//                    bt4.Delete_AllRecor(this);
//                }
                step[0]++;

            }
        });
    }
    private void saveLP4(ArrayList<Byte> file_data) {
        new Thread(() -> {
            String date = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).format(System.currentTimeMillis());
            String folderName = "Revlis_ID_Detect"; // 資料夾名稱
            String fileName = "[" + date + "]revlis.lp4";

            try {
                File folder = new File(Environment.getExternalStorageDirectory(), folderName);

                // 如果資料夾不存在，建立
                if (!folder.exists()) {
                    folder.mkdirs();
                }

                File fileLocation = new File(folder, fileName);
                FileOutputStream fos = new FileOutputStream(fileLocation);
                byte[] lp4Text = new byte[file_data.size()];
                for (int i = 0; i < file_data.size(); i++) {
                    lp4Text[i] = file_data.get(i);
                }
                fos.write(lp4Text);
                fos.close();

                MediaScannerConnection.scanFile(this, new String[]{fileLocation.getAbsolutePath()}, null, null);
                String savedFileName = fileLocation.getName();
                String savedFilePath = fileLocation.getAbsolutePath();
                initCheck2(savedFileName,savedFilePath);
                ShowToast("檔案已儲存");
            } catch (IOException e) {
                e.printStackTrace();
            }
        }).start();
    }

    public static String encodeFileToBase64Binary(ArrayList<Byte> datalist) {
        byte[] bytes = new byte[datalist.size()];
        for (int i = 0; i < bytes.length; i++) {
            bytes[i] = (byte) datalist.get(i);
        }
        return Base64.encodeToString(bytes, Base64.DEFAULT);
    }

    public void initchart()
    {
        //圖表設定2（開始頁）
        chart1.setData(new LineData());
        chart1.getXAxis().setValueFormatter(null);
        chart1.getXAxis().setDrawLabels(false);
        chart1.getXAxis().setDrawGridLines(false);
        chart1.getAxisLeft().setDrawGridLines(false);
        chart1.getAxisLeft().setDrawAxisLine(false);
        chart1.getAxisLeft().setGranularityEnabled(false);
        chart1.getXAxis().setDrawAxisLine(false);
        chart1.getAxisLeft().setDrawLabels(false);
        chart1.getAxisLeft().setAxisMinimum(1500);
        chart1.getAxisLeft().setAxisMaximum(2500);
        chart1.setScaleEnabled(false);
        chart1.setScaleXEnabled(true);
        chart1.getXAxis().setAxisMinimum(0);
        chart1.getXAxis().setAxisMaximum(300);
        chart1.getXAxis().setGranularity(30);
        chart1.getAxisLeft().setGranularity(250);
        chart1.getAxisRight().setDrawLabels(false);
        chart1.getAxisRight().setDrawGridLines(false);
        chart1.getAxisRight().setDrawAxisLine(false);
        chart1.getDescription().setText("");
        chart1.getLegend().setEnabled(false);
        chartSet1.setColor(Color.BLACK);
        chartSet1.setDrawCircles(false);
        chartSet1.setDrawFilled(false);
        chartSet1.setFillAlpha(0);
        chartSet1.setCircleRadius(0);
        chartSet1.setLineWidth((float) 1.5);
        chartSet1.setDrawValues(false);
        chartSet1.setDrawFilled(true);
    }

    public native int anaEcgFile(String name, String path);

    public native int decpEcgFile(String path);
}