package com.example.newidentify;

import static java.lang.Math.abs;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.media.MediaScannerConnection;
import android.os.Build;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.github.mikephil.charting.listener.BarLineChartTouchListener;
import com.google.gson.Gson;
import com.obsez.android.lib.filechooser.ChooserDialog;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public class MainActivity extends AppCompatActivity {
    private SharedPreferences preferences;
    private SharedPreferences.Editor editor;
    /**
     * UI
     **/
    Button btn_choose, btn_detect, btn_stop;
    TextView txt_file, txt_result, txt_value, txt_count;


    /**
     * Parameter
     **/
    private String path, filePath, fileName, ans, readTxt;
    private int dataCollectionLimit = 5;//設定檔案收集數量，最高20
    private int ccccc;
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

    // Used to load the 'newidentify' library on application startup.
    static {
        System.loadLibrary("newidentify");
        System.loadLibrary("lp4tocha");
    }

    /**
     * BLE
     */
    public static Activity global_activity;
    public static TextView txt_countDown;
    static BT4 bt4;
    TinyDB tinyDB;
    public static TextView txt_BleStatus;
    //    public static TextView Percent_Text;
    public static LineChart lineChart;
    ///////////////////////
    //////畫心電圖使用///////
    //////////////////////
    private Handler countDownHandler = new Handler();
    private boolean isToastShown = false;
    public CountDownTimer countDownTimer;//倒數
    boolean isCountDownRunning = false;
    boolean isDetectOver = false;
    private static final int COUNTDOWN_INTERVAL = 1000;
    private static final int COUNTDOWN_TOTAL_TIME = 30000;

    public static ArrayList<Entry> chartSet1Entries = new ArrayList<Entry>();
    public static ArrayList<Double> oldValue = new ArrayList<Double>();
    public static LineDataSet chartSet1 = new LineDataSet(null, "");
    static double[] mX = {0.0, 0.0, 0.0, 0.0, 0.0};
    static double[] mY = {0.0, 0.0, 0.0, 0.0, 0.0};
    static int[] mStreamBuf = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    static double[] mAcoef = {0.00001347408952448771,
            0.00005389635809795083,
            0.00008084453714692624,
            0.00005389635809795083,
            0.00001347408952448771};
    static double[] mBcoef = {1.00000000000000000000,
            -3.67172908916193470000,
            5.06799838673418980000,
            -3.11596692520174570000,
            0.71991032729187143000};

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        preferences = getSharedPreferences("my_preferences", MODE_PRIVATE);
        editor = preferences.edit();
        global_activity = this;
        bt4 = new BT4(global_activity);
        tinyDB = new TinyDB(this);

        lineChart = findViewById(R.id.linechart);
        initchart();
        initObject();
        initPermission();
        checkStorageManagerPermission();
    }

    @Override
    protected void onResume() {
        super.onResume();
        initChooser();
        initBroadcast();
        bt4.Bluetooth_init();
        setScreenOn();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (countDownTimer != null) {
            stopCountdownWave();
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
        setScreenOff();
        if (bt4.isconnect) {
            bt4.close();
        }
        unregisterReceiver(getReceiver);
        stopCountdownWave();
    }

    private void initBroadcast() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BT4.BLE_CONNECTED);
        intentFilter.addAction(BT4.BLE_TRY_CONNECT);
        intentFilter.addAction(BT4.BLE_DISCONNECTED);
        intentFilter.addAction(BT4.BLE_READ_FILE);
        registerReceiver(getReceiver, intentFilter);
    }

    /**
     * 藍芽已連接/已斷線資訊回傳
     */
    private final BroadcastReceiver getReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (BT4.BLE_CONNECTED.equals(action)) {
                txt_BleStatus.setText("已連線");
            } else if (BT4.BLE_DISCONNECTED.equals(action)) {
                txt_BleStatus.setText("已斷線");
            } else if (BT4.BLE_TRY_CONNECT.equals(action)) {
                Toast.makeText(global_activity, "搜尋到裝置，連線中", Toast.LENGTH_SHORT).show();
            } else if (BT4.BLE_READ_FILE.equals(action)) {
                txt_countDown.setText((bt4.file_data.size() * 100 / bt4.File_Count) + " %");
            }
        }
    };

    /**
     * 螢幕恆亮
     **/
    public void setScreenOn() {
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    public void setScreenOff() {
        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    private void initObject() {
        btn_stop = findViewById(R.id.btn_stop);
        btn_detect = findViewById(R.id.btn_detect);
        btn_choose = findViewById(R.id.btn_choose);
        txt_file = findViewById(R.id.txt_file);
        txt_result = findViewById(R.id.txt_result);
        txt_value = findViewById(R.id.txt_value);
        txt_count = findViewById(R.id.txt_count);
        txt_countDown = findViewById(R.id.txt_countDown);
        txt_BleStatus = findViewById(R.id.txt_BleStatus);
        try {
            File file = new File(filePath);
            if (file.mkdir()) {
                System.out.println("新增資料夾");
            } else {
                System.out.println("資料夾已存在");
            }
        } catch (Exception e) {
            Log.e("where", e.toString());
        }
        btn_stop.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                stopCountdownWave();
            }
        });
    }

    /**
     * 檢查權限
     **/
    public void initPermission() {
        if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{android.Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1000);
        }
        if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 1);
        }

        List<String> mPermissionList = new ArrayList<>();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            mPermissionList.add(Manifest.permission.BLUETOOTH_SCAN);
            mPermissionList.add(Manifest.permission.BLUETOOTH_ADVERTISE);
            mPermissionList.add(Manifest.permission.BLUETOOTH_CONNECT);
        } else {
            mPermissionList.add(Manifest.permission.ACCESS_COARSE_LOCATION);
            mPermissionList.add(Manifest.permission.ACCESS_FINE_LOCATION);
            mPermissionList.add(Manifest.permission.MANAGE_EXTERNAL_STORAGE);
        }
        ActivityCompat.requestPermissions(this, mPermissionList.toArray(new String[0]), 1001);
    }

    private void checkStorageManagerPermission() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R || Environment.isExternalStorageManager()) {
        } else {
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setMessage("本程式需要您同意允許存取所有檔案權限");
            builder.setPositiveButton("同意", new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    Intent intent = new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                    startActivity(intent);
                }
            });
            builder.show();
        }
    }

    /**
     * 檔案選擇器
     **/
    public void initChooser() {
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                //找外部儲存
                String externalStorageDirectory = Environment.getExternalStorageDirectory().getAbsolutePath();
//                String externalStorageDirectory = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getPath();
                chooserDialog = new ChooserDialog(MainActivity.this)
                        .withStartFile(String.valueOf(externalStorageDirectory + "/Apple_ID_Detect"))
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

                                path = filePath.substring(0, filePath.length() - fileName.length());
                                txt_file.setText(fileName);

                                initCheck();
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


    /**
     * ID識別按鈕事件
     **/
    private void initCheck() {
        if (fileName != null) {
            if (fileName.endsWith(".lp4")) {
                decpEcgFile(filePath);
                int u = fileName.length();
                String j = fileName.substring(0, u - 4);
                fileName = j + ".cha";
                initIdentify();
            } else if (fileName.endsWith(".cha") | fileName.endsWith(".CHA")) {
                initIdentify();
            } else {
                txt_result.setText("不支援此檔案類型");
            }
        } else {
            txt_result.setText("尚未選擇檔案");
        }
    }

    /**
     * 執行c++檔
     **/
    private void initIdentify() {
        int x = anaEcgFile(fileName, path);
        if (x == 1) {
            txt_result.setText("檔案訊號error，請換個檔案繼續");
        } else {
//            ccccc += 1;
        }
        filePath = path;
        fileName = fileName.substring(0, fileName.length() - 4);
        try {
            File file = new File(filePath + "/r_" + fileName + ".txt");
            if (file.isFile() && file.exists()) {
                BufferedReader reader = new BufferedReader(new FileReader(file));
                String line;
                while ((line = reader.readLine()) != null) {
//                    txt_result.setText(line);
                    readTxt = line;
                }
                line = readTxt;
                String[] parts = line.split(",");

                for (String part : parts) {
                    String[] nameValue = part.split(":");
                    if (nameValue.length == 2) {
                        String name = nameValue[0];
                        String value = nameValue[1];
                        dataMap.put(name, value);
                    }
                }
                ValueHR = Double.parseDouble(dataMap.get("Average"));
                ValuePI = Double.parseDouble(dataMap.get("PI"));
                ValueCvi = Double.parseDouble(dataMap.get("CVI"));
                ValueC1a = Double.parseDouble(dataMap.get("C1a"));
                if (ValueHR > 50 && ValueHR < 150) {
                    if (heartRate.size() < dataCollectionLimit) {//如果數據數小於dataCollectionLimit就繼續收集
                        heartRate.add(ValueHR);//把LP4算好的結果加進List
                        PI.add(ValuePI);
                        CVI.add(ValueCvi);
                        C1a.add(ValueC1a);
                        getValue();
                    }
                } else {
//                    ccccc -= 1;
                    ShowToast("訊號品質不好，請重新量測");
                }
                if (heartRate.size() == dataCollectionLimit) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            ShowToast("註冊成功...");
                            saveInfoToPreference();
                            signUpDialog();
                        }
                    });
                }
                String s = String.format("HR: %s \n PI: %s \n CVI: %s \n C1a: %s", heartRate.toString(), PI.toString(), CVI.toString(), C1a.toString());
                Log.d("getListSize", "HR" + String.valueOf(heartRate.size()));
                Log.d("getListSize", "DC" + dataCollectionLimit);
                Log.d("ListValue", s);
                if (ans != null) {
                    txt_result.setText(ans);
                } else {
                    txt_result.setText("檔案數量不足");
                }
                txt_value.setText(s);
                txt_count.setText(String.format("目前設定的檔案數量: %d\n輸入檔案數量: %d", dataCollectionLimit, heartRate.size()));
                reader.close();
            }
        } catch (Exception e) {
            ShowToast("訊號品質不好，請重新量測");
            Log.e("catchError", e.toString());
        }
    }

    private void getValue() {//算各項數值的平均值
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
    private void judgeValue() {
        for (Double value : heartRate) {
            if (value > 100 || value > averageHR * 1.2) {
                checkX = true;
                Log.d("HRValue", String.format("x: %b\nhr: %f\naverageHR: %f\naverageHR*1.2: %f", checkX, value, averageHR, averageHR * 1.2));
                break;
            } else {
                checkX = false;
                Log.d("HRValue", String.format("averageHR: %f\naverageHR*1.2: %f", averageHR, averageHR * 1.2));
            }
        }
        if (checkX || maxPI - minPI > 0.25 || maxCVI - minCVI > 4.5 || abs(maxC1a - averageC1a) >= 20 || abs(minC1a - averageC1a) >= 20) {
            ans = identifyPlan.Second(averageHR, ValueHR, averagePI, ValuePI, averageCVI, ValueCvi, averageC1a, ValueC1a);
        } else {
            ans = identifyPlan.First(averageHR, ValueHR, averagePI, ValuePI, averageCVI, ValueCvi, averageC1a, ValueC1a);
        }
        try {
            if (ans.equals("本人")) {
                Log.d("ListValue", dataMap.get("Average"));
                heartRate.set(heartRate.size() % dataCollectionLimit - 1, ValueHR);
                PI.set(heartRate.size() % dataCollectionLimit - 1, ValuePI);
                CVI.set(heartRate.size() % dataCollectionLimit - 1, ValueCvi);
                C1a.set(heartRate.size() % dataCollectionLimit - 1, ValueC1a);
            } else {
                newDialog();
            }
        } catch (Exception e) {
            Log.e("super", e.toString());
        }
    }

    private void newDialog() {
        AlertDialog.Builder alertDialog = new AlertDialog.Builder(MainActivity.this);
        alertDialog.setTitle("是否為帳號本人使用？");
        alertDialog.setPositiveButton("是", ((dialog, which) -> {
        }));
        alertDialog.setNegativeButton("否", ((dialog, which) -> {
        }));
        AlertDialog dialog = alertDialog.create();
        dialog.show();
        dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener((v -> {
            switch (dataCollectionLimit) {
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

            String s = String.format("HR: %s\nPI: %s\nCVI: %s\nC1a: %s", heartRate.toString(), PI.toString(), CVI.toString(), C1a.toString());
            txt_count.setText(String.format("目前設定的檔案數量: %d\n輸入檔案數量: %d", dataCollectionLimit, heartRate.size()));
            txt_result.setText("檔案數量不足");
            txt_value.setText(s);

            dialog.dismiss();
        }));
        dialog.getButton(AlertDialog.BUTTON_NEGATIVE).setOnClickListener((v -> {
            txt_result.setText("非本人");
            dialog.dismiss();
        }));

        dialog.setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);
    }

    private void signUpDialog() {
        AlertDialog.Builder alertDialog = new AlertDialog.Builder(MainActivity.this);
        alertDialog.setTitle("是否回到登入頁面？");
        alertDialog.setPositiveButton("是", (dialog, which) -> {
            // 跳轉到 MainActivity
            dialog.dismiss();

            Intent intent = new Intent(MainActivity.this, LoginActivity.class);
            startActivity(intent);
            finish();
        });
        alertDialog.setNegativeButton("否", (dialog, which) -> {
            dialog.dismiss();
        });
        AlertDialog dialog = alertDialog.create();
        dialog.show();

        dialog.setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);
    }

    public void saveInfoToPreference() {

        tinyDB.putInt("count", heartRate.size());

        // 儲存Map
        Gson gson = new Gson();
        String dataMapJson = gson.toJson(dataMap);
        editor.putString("dataMap", dataMapJson);

        tinyDB.putListDouble("heartRate", heartRate);
        tinyDB.putListDouble("PI", PI);
        tinyDB.putListDouble("CVI", CVI);
        tinyDB.putListDouble("C1a", C1a);

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
                    if (bt4.isTenSec) {
                        chartSet1Entries.add(chartSet1Entrie);
                        chartSet1.setValues(chartSet1Entries);
                        lineChart.setData(new LineData(chartSet1));
                        lineChart.setVisibleXRangeMinimum(300);
                        lineChart.invalidate();
                    }

                    for (Entry entry : chartSet1Entries) {
                        float xValue = entry.getX();
                        float yValue = entry.getY();
                        Log.d("ssss", "run: " + yValue);
                    }

                }
            });
        } catch (Exception ex) {
//            Log.d("wwwww", "eeeeeerrr = " + ex.toString());
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


    /**
     * 量測與畫圖
     */

    @SuppressLint("HandlerLeak")
    public void RecordWaveAction(View view) {
        bt4.Bluetooth_init();
        if (bt4.isconnect) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    lineChart.clear();
                    lineChart.invalidate();
                    initchart();
                }
            });
            bt4.Wave(true, new Handler());

            if (!isCountDownRunning) {
                isDetectOver = false;
                startCountDown();
            }
        }
    }

    private void startCountDown() {
        isCountDownRunning = true;
        isDetectOver = false;
        countDownHandler.postDelayed(new Runnable() {
            private int presetTime = 10000;
            private int remainingTime = COUNTDOWN_TOTAL_TIME;

            @Override
            public void run() {
                if (!isToastShown) {
                    Toast.makeText(global_activity, "請將手指放置於裝置上，量測馬上開始", Toast.LENGTH_LONG).show();
                    isToastShown = true;
                }
                if (presetTime <= 0) {//倒數6秒結束才開始跑波
                    bt4.isTenSec = true;
                    if (remainingTime <= 0) {//結束的動作
                        txt_countDown.setText("30");
                        stopWave();
                        isCountDownRunning = false;
                        isDetectOver = true;
                    } else {//秒數顯示
                        txt_countDown.setText(String.valueOf(remainingTime / 1000));
                        remainingTime -= COUNTDOWN_INTERVAL;
                        countDownHandler.postDelayed(this, COUNTDOWN_INTERVAL);
                    }
                } else {
                    bt4.isTenSec = false;
                    txt_countDown.setText(String.valueOf(presetTime / 1000));
                    presetTime -= COUNTDOWN_INTERVAL;
                    countDownHandler.postDelayed(this, COUNTDOWN_INTERVAL);

                }
            }
        }, COUNTDOWN_INTERVAL);
    }

    private void stopCountdownWave() {
        if (isCountDownRunning) {
            countDownHandler.removeCallbacksAndMessages(null); // 移除倒數
            isCountDownRunning = false;
            txt_countDown.setText("30");
            stopWave();
        }
    }

    @SuppressLint("HandlerLeak")
    public void stopWave() {
        if (bt4.isWave) {
            ShowToast("正在停止跑波...");
            bt4.StopMeasure(new Handler() {
                @Override
                public void handleMessage(Message msg2) {
                    ShowToast("完成停止跑波");
                    if (isDetectOver) {
                        readFile();
                    }
                }
            });
        }
    }


    @SuppressLint("HandlerLeak")
    private void readFile() {
        final int[] step = {0};
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
                }

                if (step[0] == 3) {
                    saveLP4(bt4.file_data);
                    bt4.Delete_AllRecor(this);
                }

                if (step[0] == 4) {
                    bt4.Delete_AllRecor(this);
                    bt4.file_data.clear();
                    bt4.Buffer_Array.clear();
                }
                step[0]++;
            }
        });
    }

    private void saveLP4(ArrayList<Byte> file_data) {

        new Thread(() -> {
            String date = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).format(System.currentTimeMillis());
            String folderName = "Apple_ID_Detect"; // 資料夾名稱
            String s = date + "RV.lp4";

            try {
                File folder = new File(Environment.getExternalStorageDirectory().getAbsolutePath(), folderName);
                // 如果資料夾不存在，建立
                if (!folder.exists()) {
                    folder.mkdirs();
                }

                File fileLocation = new File(folder, s);
                FileOutputStream fos = new FileOutputStream(fileLocation);
                byte[] lp4Text = new byte[file_data.size()];
                for (int i = 0; i < file_data.size(); i++) {
                    lp4Text[i] = file_data.get(i);
                    Log.d("pppp", "lp4Text: " + file_data.get(i));
                }
                fos.write(lp4Text);
                fos.close();

                MediaScannerConnection.scanFile(this, new String[]{fileLocation.getAbsolutePath()}, null, null);
                String savedFilePath = fileLocation.getAbsolutePath();
                ShowToast("檔案已儲存");
                Log.d("gggg", "saveLP4: " + savedFilePath);
                setChooseFile(savedFilePath);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }).start();
    }

    public void setChooseFile(String Apath) {

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                autoDetect(Apath);
            }
        });
    }

    public void autoDetect(String dir) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                filePath = dir;
                File file = new File(dir);
                fileName = file.getName();

                path = filePath.substring(0, filePath.length() - fileName.length());
                txt_file.setText(fileName);

                initCheck();
            }
        });

    }


    public void initchart() {
        // 允許滑動
        lineChart.setDragEnabled(true);

        // 設定縮放
        lineChart.setScaleEnabled(false);
        lineChart.setPinchZoom(false);

        // 其他圖表設定
        lineChart.setData(new LineData());
        lineChart.getXAxis().setValueFormatter(null);
        lineChart.getXAxis().setDrawLabels(false);
        lineChart.getXAxis().setDrawGridLines(false);
        lineChart.getAxisLeft().setDrawGridLines(false);
        lineChart.getAxisLeft().setDrawAxisLine(false);
        lineChart.getAxisLeft().setGranularityEnabled(false);
        lineChart.getXAxis().setDrawAxisLine(false);
        lineChart.getAxisLeft().setDrawLabels(false);
        lineChart.getAxisLeft().setAxisMinimum(1500);
        lineChart.getAxisLeft().setAxisMaximum(2500);
        lineChart.getXAxis().setAxisMinimum(0);
        lineChart.getXAxis().setAxisMaximum(300);
        lineChart.getXAxis().setGranularity(30);
        lineChart.getAxisLeft().setGranularity(250);
        lineChart.getAxisRight().setDrawLabels(false);
        lineChart.getAxisRight().setDrawGridLines(false);
        lineChart.getAxisRight().setDrawAxisLine(false);
        lineChart.getDescription().setText("");
        lineChart.getLegend().setEnabled(false);
        chartSet1.setColor(Color.BLACK);
        chartSet1.setDrawCircles(false);
        chartSet1.setDrawFilled(false);
        chartSet1.setFillAlpha(0);
        chartSet1.setCircleRadius(0);
        chartSet1.setLineWidth((float) 1.5);
        chartSet1.setDrawValues(false);
        chartSet1.setDrawFilled(true);

        float scaleX = lineChart.getScaleX();
        if (scaleX == 1)
            lineChart.zoomToCenter(5, 1f);
        else {
            BarLineChartTouchListener barLineChartTouchListener = (BarLineChartTouchListener) lineChart.getOnTouchListener();
            barLineChartTouchListener.stopDeceleration();
            lineChart.fitScreen();
        }

        lineChart.invalidate();

    }

    public static native int anaEcgFile(String name, String path);

    public static native int decpEcgFile(String path);


}