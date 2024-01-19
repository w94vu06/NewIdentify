package com.example.newidentify;

import static com.example.newidentify.processData.SignalProcess.Butterworth;
import static java.lang.Math.abs;
import static java.lang.Math.getExponent;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.StrictMode;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;

import com.example.newidentify.Util.ChartSetting;
import com.example.newidentify.Util.CleanFile;
import com.example.newidentify.processData.BpmCountThread;
import com.example.newidentify.processData.DecodeCHAFile;
import com.example.newidentify.processData.SignalProcess;
import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.XAxis;
import com.github.mikephil.charting.components.YAxis;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.github.mikephil.charting.listener.BarLineChartTouchListener;
import com.obsez.android.lib.filechooser.ChooserDialog;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;


public class MainActivity extends AppCompatActivity {
    private SharedPreferences preferences;
    private SharedPreferences.Editor editor;
    /**
     * UI
     **/
    Button btn_detect, btn_stop_clean;
    TextView txt_fileName, txt_result;

    /**
     * choose Device Dialog
     */

    Dialog deviceDialog;

    /**
     * Parameter
     **/
    private String fileName;
    private String filePath;
    private String path;

    private String getfileName;
    private String getFilePath = "";
    private final int dataCollectionLimit = 5;//設定檔案收集數量，最高20
    private ChooserDialog chooserDialog; //檔案選擇器
    private final Map<String, String> dataMap = new HashMap<>();
    private final ArrayList<Double> heartRate = new ArrayList<>();
    private final ArrayList<Double> PI = new ArrayList<>();
    private final ArrayList<Double> CVI = new ArrayList<>();
    private final ArrayList<Double> C1a = new ArrayList<>();
    private static final ArrayList<Float> saveWaveData = new ArrayList<>();
    double averageHR, averagePI, averageCVI, averageC1a;
    double maxPI, maxCVI, maxC1a;
    double minPI, minCVI, minC1a;
    double ValueHR, ValuePI, ValueCvi, ValueC1a;

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
    public static LineChart chart_df;
    private ChartSetting chartSetting;
    ///////////////////////
    //////畫心電圖使用///////
    //////////////////////
    private final Handler countDownHandler = new Handler();
    private boolean isToastShown = false;
    public CountDownTimer countDownTimer;//倒數
    boolean isCountDownRunning = false;
    boolean isMeasurementOver = false;
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
    /**
     * L2D
     */
    private DecodeCHAFile decodeCHAFile;
    private SignalProcess signalProcess;
    private BpmCountThread bpmCountThread;
    private CleanFile cleanFile;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        //初始化SharedPreference
        preferences = getSharedPreferences("my_preferences", MODE_PRIVATE);
        editor = preferences.edit();

        global_activity = this;
        bt4 = new BT4(global_activity);
        tinyDB = new TinyDB(global_activity);
        deviceDialog = new Dialog(global_activity);
        signalProcess = new SignalProcess();
        cleanFile = new CleanFile();
        chartSetting = new ChartSetting();
        lineChart = findViewById(R.id.linechart);
        chart_df = findViewById(R.id.chart_df);

        initchart();//初始化圖表
        initObject();//初始化物件
        initPermission();//檢查權限
        checkStorageManagerPermission();//檢查儲存權限
        initDeviceDialog();//裝置選擇Dialog
    }

    @Override
    protected void onResume() {
        super.onResume();
        initBroadcast();
        setScreenOn();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (countDownTimer != null) {
            stopCountdownWaveMeasurement();
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
        setScreenOff();
        if (bt4.isConnected) {
            bt4.close();
        }
        unregisterReceiver(getReceiver);
        stopCountdownWaveMeasurement();
    }

    @SuppressLint("UnspecifiedRegisterReceiverFlag")
    private void initBroadcast() {
        //註冊廣播過濾
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
        btn_stop_clean = findViewById(R.id.btn_stop_clean);
        btn_detect = findViewById(R.id.btn_detect);
        txt_fileName = findViewById(R.id.txt_fileName);
        txt_result = findViewById(R.id.txt_result);
        txt_countDown = findViewById(R.id.txt_countDown);
        txt_BleStatus = findViewById(R.id.txt_BleStatus);

        btn_stop_clean.setOnClickListener(view -> stopCountdownWaveMeasurement());//停止量測Btn
    }

    /**
     * 初始化裝置選擇Dialog
     **/
    public void initDeviceDialog() {
        deviceDialog.setContentView(R.layout.dialog_device);
        // 初始化元件
        RadioGroup devicesRadioGroup = deviceDialog.findViewById(R.id.devicesRadioGroup);
        RadioButton radioButtonDevice1 = deviceDialog.findViewById(R.id.radioButtonDevice1);
        RadioButton radioButtonDevice2 = deviceDialog.findViewById(R.id.radioButtonDevice2);
        Button completeButton = deviceDialog.findViewById(R.id.completeButton);
        deviceDialog.setCancelable(false);

        // 設置按鈕點擊監聽器
        completeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                // 在這裡處理完成按鈕的點擊事件
                // 檢查哪個 RadioButton 被選中
                int checkedRadioButtonId = devicesRadioGroup.getCheckedRadioButtonId();
                if (checkedRadioButtonId == R.id.radioButtonDevice1) {
                    bt4.deviceName = "CmateH";
                    bt4.Bluetooth_init();
                    deviceDialog.dismiss();

                } else if (checkedRadioButtonId == R.id.radioButtonDevice2) {
                    bt4.deviceName = "WTK230";
                    bt4.Bluetooth_init();
                    deviceDialog.dismiss();
                    // 處理選中 Device 2 的邏輯
                } else {
                    // 提示用戶選擇一個裝置
                    Toast.makeText(global_activity, "請選擇一個裝置", Toast.LENGTH_SHORT).show();
                }
            }
        });

        // 顯示對話框
        deviceDialog.show();
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

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == 350) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Log.d("wwwww", "搜尋設備");
            }
        }
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
                    if (oldValue.size() > 120) {
                        Entry chartSet1Entrie = new Entry(chartSet1Entries.size(), (float) nvalue);
                        chartSet1Entries.add(chartSet1Entrie);
                        saveWaveData.add((float) nvalue);
                    }
                    chartSet1.setValues(chartSet1Entries);
                    lineChart.setData(new LineData(chartSet1));
                    lineChart.setVisibleXRangeMinimum(300);
                    lineChart.invalidate();
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
    public void startWaveMeasurement(View view) {
        bt4.Bluetooth_init();
        if (bt4.isConnected) {
            runOnUiThread(() -> {
                lineChart.clear();
                initchart();
            });

            bt4.startWave(true, new Handler());

            if (!isCountDownRunning) {
                isMeasurementOver = false;
                startCountDown();
            }
        } else {
            ShowToast("請先連接裝置");
        }
    }

    private void startCountDown() {
        isCountDownRunning = true;
        isMeasurementOver = false;
        countDownHandler.postDelayed(new Runnable() {
            private int presetTime = 6000;
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
                        stopWaveMeasurement();
                        isCountDownRunning = false;
                        isMeasurementOver = true;
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

    private void stopCountdownWaveMeasurement() {
        if (isCountDownRunning) {
            countDownHandler.removeCallbacksAndMessages(null); // 移除倒數
            isCountDownRunning = false;
            txt_countDown.setText("30");
            stopWaveMeasurement();
            cleanRegisterFile();
        }
    }

    private void cleanRegisterFile() {
        tinyDB.clear();
        cleanFile.cleanFile(getFilePath);
    }

    @SuppressLint("HandlerLeak")
    public void stopWaveMeasurement() {
        if (bt4.isWave) {
            ShowToast("正在停止跑波...");
            bt4.StopMeasure(new Handler() {
                @Override
                public void handleMessage(Message msg2) {
                    ShowToast("完成停止跑波");

                    if (isMeasurementOver) {
                        processMeasurementData();
                    }
                }
            });
        } else {
            ShowToast("尚未開始跑波");
        }
    }

    /**
     * 處理量測檔案
     */
    @SuppressLint("HandlerLeak")
    private void processMeasurementData() {
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
                    if (bt4.file_data.size() > 0) {
                        saveLP4(bt4.file_data);
                    } else {
                        ShowToast("檔案大小為0");
                    }
                    bt4.Delete_AllRecor(this);
                }

                if (step[0] == 4) {
                    readLP4(getFilePath);

                    bt4.Delete_AllRecor(this);
                    bt4.file_data.clear();
                    bt4.Buffer_Array.clear();
                }
                step[0]++;
            }
        });
    }

    private void saveLP4(ArrayList<Byte> file_data) {
        try {
            String date = new SimpleDateFormat("yyyyMMddHHmmss", Locale.getDefault()).format(System.currentTimeMillis());
            String fileName = "r_" + date + "_888888.lp4";
            getfileName = fileName;

            // 直接在外部存儲目錄中創建文件
            File fileLocation = new File(Environment.getExternalStorageDirectory().getAbsolutePath(), fileName);

            saveByteArrayToFile(file_data, fileLocation);

            runOnUiThread(() -> {
                // 在主執行緒中處理 UI 相關的操作
                ShowToast("LP4儲存成功");
                MediaScannerConnection.scanFile(this, new String[]{fileLocation.getAbsolutePath()}, null, (path, uri) -> {
                    getFilePath = path;
                });
            });
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    //byte to double
    private void makeCSV(ArrayList<Double> doubles, String fileName) {
        new Thread(() -> {
            /** 檔名 */
            String date = new SimpleDateFormat("yyyyMMddhhmmss",
                    Locale.getDefault()).format(System.currentTimeMillis());
            String[] title = {"Lead2"};
            StringBuffer csvText = new StringBuffer();
            for (int i = 0; i < title.length; i++) {
                csvText.append(title[i] + ",");
            }
            /** 內容 */
            for (int i = 0; i < doubles.size(); i++) {
                csvText.append("\n" + doubles.get(i));
            }

            runOnUiThread(() -> {
                try {
                    StrictMode.VmPolicy.Builder builder = new StrictMode.VmPolicy.Builder();
                    StrictMode.setVmPolicy(builder.build());
                    builder.detectFileUriExposure();
                    FileOutputStream out = openFileOutput(fileName, Context.MODE_PRIVATE);
                    out.write((csvText.toString().getBytes()));
                    out.close();
                    File fileLocation = new File(Environment.getExternalStorageDirectory().getAbsolutePath(), fileName);
                    FileOutputStream fos = new FileOutputStream(fileLocation);
                    fos.write(csvText.toString().getBytes());
                    Uri path = Uri.fromFile(fileLocation);
                    Intent fileIntent = new Intent(Intent.ACTION_SEND);
                    fileIntent.setType("text/csv");
                    fileIntent.putExtra(Intent.EXTRA_SUBJECT, fileName);
                    fileIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                    fileIntent.putExtra(Intent.EXTRA_STREAM, path);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            });
        }).start();
    }//makeCSV

    private void makeCSVFloat(List<Float> df1, List<Float> df2, List<Float> df3, List<Float> df4, String fileName) {
        new Thread(() -> {
            /** 檔名 */
            String date = new SimpleDateFormat("yyyyMMddhhmmss", Locale.getDefault()).format(System.currentTimeMillis());
            String[] titles = {"df1", "df2", "df3", "df4"};
            StringBuffer csvText = new StringBuffer();

            for (int i = 0; i < titles.length; i++) {
                csvText.append(titles[i] + ",");
            }

            int maxSize = Math.max(Math.max(df1.size(), df2.size()), Math.max(df3.size(), df4.size()));
            for (int i = 0; i < maxSize; i++) {

                if (i < df1.size()) {
                    csvText.append(df1.get(i));
                }
                csvText.append(",");

                if (i < df2.size()) {
                    csvText.append(df2.get(i));
                }
                csvText.append(",");

                if (i < df3.size()) {
                    csvText.append(df3.get(i));
                }
                csvText.append(",");

                if (i < df4.size()) {
                    csvText.append(df4.get(i));
                }

                csvText.append("\n");
            }

            runOnUiThread(() -> {
                try {
                    StrictMode.VmPolicy.Builder builder = new StrictMode.VmPolicy.Builder();
                    StrictMode.setVmPolicy(builder.build());
                    builder.detectFileUriExposure();
                    FileOutputStream out = openFileOutput(fileName, Context.MODE_PRIVATE);
                    out.write((csvText.toString().getBytes()));
                    out.close();
                    File fileLocation = new File(Environment.getExternalStorageDirectory().getAbsolutePath(), fileName);
                    FileOutputStream fos = new FileOutputStream(fileLocation);
                    fos.write(csvText.toString().getBytes());
                    Uri path = Uri.fromFile(fileLocation);
                    Intent fileIntent = new Intent(Intent.ACTION_SEND);
                    fileIntent.setType("text/csv");
                    fileIntent.putExtra(Intent.EXTRA_SUBJECT, fileName);
                    fileIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                    fileIntent.putExtra(Intent.EXTRA_STREAM, path);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            });
        }).start();
    }

    /**
     * 儲存檔案
     */
    private void saveByteArrayToFile(ArrayList<Byte> byteList, File file) throws IOException {
        byte[] byteArray = new byte[byteList.size()];
        for (int i = 0; i < byteList.size(); i++) {
            Byte byteValue = byteList.get(i);
            byteArray[i] = (byteValue != null) ? byteValue : 0;
        }

        try (FileOutputStream fos = new FileOutputStream(file)) {
            fos.write(byteArray);
        }
    }

    private void readLP4(String filePath) {
        File file = new File(filePath);
        decpEcgFile(filePath);
        String fileName = file.getName();
        int y = fileName.length();
        String j = fileName.substring(0, y - 4);
        fileName = j + ".CHA"; // Ensure the consistent file extension

        // Debugging: Output file paths
        Log.d("FilePaths", "LP4 Path: " + filePath);
        Log.d("FilePaths", "CHA Path: " + fileName);

        readCHA(fileName);
    }

    /**
     * 讀取CHA
     */
    public void readCHA(String fileName) {
        // Debugging: Output CHA file path
        Log.d("FilePaths", "CHA Path in readCHA: " + fileName);

        if (!fileName.isEmpty()) {
            File chaFile = new File(Environment.getExternalStorageDirectory(), fileName);
            Log.d("FilePaths", "chaFile: " + chaFile);
            if (chaFile.exists()) {
                decodeCHAFile = new DecodeCHAFile(chaFile.getAbsolutePath());
                try {
                    decodeCHAFile.run();
                    decodeCHAFile.join();
                    ShowToast("匯入CHA成功");
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                calculateRR(decodeCHAFile.finalCHAData);
            } else {
                Log.e("CHAFileNotFound", "CHA檔案不存在：" + fileName);
            }
        } else {
            Log.e("EmptyFileName", "文件名稱為空");
        }
    }

    private void calculateRR(List<Float> dataList) {
        if (dataList == null || dataList.size() == 0) {
            Log.e("EmptyDataList", "dataList為空");
        } else {
            bpmCountThread = new BpmCountThread(dataList);
            bpmCountThread.run();
            //makeCsv  ArrayList<Double>
            //resultFloatArray Float[]
            ArrayList<Double> doubles = floatArrayToDoubleList(bpmCountThread.resultFloatArray);
            String date = new SimpleDateFormat("yyyyMMddhhmmss",
                    Locale.getDefault()).format(System.currentTimeMillis());
            makeCSV(doubles, "original_" + date + ".csv");
            calMidError(bpmCountThread.resultFloatArray);
        }
    }

    public void calMidError(Float[] floats) {
        List<Integer> R_index = bpmCountThread.R_index_up;
        if (R_index.size() > 10) {
            //取得已經過濾過的RR 100(4張圖)
            List<Float> df1 = signalProcess.getReduceRR100(Arrays.asList(floats), R_index.get(10), R_index.get(12));
            List<Float> df2 = signalProcess.getReduceRR100(Arrays.asList(floats), R_index.get(3), R_index.get(5));
            List<Float> df3 = signalProcess.getReduceRR100(Arrays.asList(floats), R_index.get(6), R_index.get(8));
            List<Float> df4 = signalProcess.getReduceRR100(Arrays.asList(floats), R_index.get(8), R_index.get(10));
            overlapChart(chart_df, df1, df2, df3, df4);

            String date = new SimpleDateFormat("yyyyMMddhhmmss", Locale.getDefault()).format(System.currentTimeMillis());
            makeCSVFloat(df1, df2, df3, df4, "df_" + date + ".csv");

            if (tinyDB.getListFloat("df1").size() < 10) {
                tinyDB.putString("chooserFileName", getfileName);
                tinyDB.putListFloat("df1", df1);
                tinyDB.putListFloat("df2", df2);
                tinyDB.putListFloat("df3", df3);
                tinyDB.putListFloat("df4", df4);
            } else {
                Log.d("TinyDB", "已經有第一段數據了");
            }
            String firstFileName = tinyDB.getString("chooserFileName");
            //輸出自己與別人的差異
            List<Float> df1_ = tinyDB.getListFloat("df1");
            List<Float> df2_ = tinyDB.getListFloat("df2");
            List<Float> df3_ = tinyDB.getListFloat("df3");
            List<Float> df4_ = tinyDB.getListFloat("df4");
            //計算自己的差異
            float diff12 = signalProcess.calMidDiff(df1, df2);
            float diff13 = signalProcess.calMidDiff(df1, df3);
            float diff14 = signalProcess.calMidDiff(df1, df4);
            float diff23 = signalProcess.calMidDiff(df2, df3);
            //計算別人的差異(只用df1)
            float diff12_ = signalProcess.calMidDiff(df1_, df2_);
            float diff13_ = signalProcess.calMidDiff(df1_, df3);
            float diff14_ = signalProcess.calMidDiff(df1_, df4);
            float diff23_ = signalProcess.calMidDiff(df2, df3);

            float averageDiff4Num_self = (diff12 + diff13 + diff14 + diff23) / 4;
            float averageDiff4Num_sb = (diff12_ + diff13_ + diff14_ + diff23_) / 4;

            if (Objects.equals(firstFileName, fileName)) {
                Log.d("record", fileName + " 自己的差異度: " + averageDiff4Num_self);
                txt_result.setText(String.format("自己的差異度: %s", averageDiff4Num_self));
            } else {
                Log.d("record", fileName + " 自己的差異度: " + averageDiff4Num_self);
                Log.d("record", firstFileName + " 與 " + fileName + "的差異度: " + averageDiff4Num_sb);
                txt_result.setText(String.format("自己的差異度: %s\n與 %s \n差異度: %s", averageDiff4Num_self, firstFileName, averageDiff4Num_sb));
            }
        }
    }

    public void saveByte(ArrayList<Byte> byteArrayList) {
        // 將 ArrayList<Byte> 轉換為 byte 數組
        byte[] byteArray = new byte[byteArrayList.size()];
        for (int i = 0; i < byteArrayList.size(); i++) {
            byteArray[i] = byteArrayList.get(i);
        }
    }

    public void backByte(byte[] byteArray) {
        // 將 byte 數組轉換為 ArrayList<Byte>
        ArrayList<Byte> byteArrayList = new ArrayList<>();
        for (byte b : byteArray) {
            byteArrayList.add(b);
        }
    }

    public ArrayList<Double> floatArrayToDoubleList(Float[] arrayList) {
        // 將 ArrayList<Byte> 轉換為 byte 數組
        ArrayList<Double> doubleArray = new ArrayList<>();
        for (int i = 0; i < arrayList.length; i++) {
            doubleArray.add(Double.valueOf(arrayList[i]));
        }
        return doubleArray;
    }

    public void initchart() {
        // 允許滑動
        chartSetting.initchart(lineChart);

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

    public void overlapChart(LineChart lineChart, List<Float> df1, List<Float> df2, List<Float> df3, List<Float> df4) {

        //list float to entry
        ArrayList<Entry> df1_ = new ArrayList<>();
        ArrayList<Entry> df2_ = new ArrayList<>();
        ArrayList<Entry> df3_ = new ArrayList<>();
        ArrayList<Entry> df4_ = new ArrayList<>();
        for (int i = 0; i < df1.size(); i++) {
            Entry entry = new Entry(i, df1.get(i));
            df1_.add(entry);
        }
        for (int i = 0; i < df2.size(); i++) {
            Entry entry = new Entry(i, df2.get(i));
            df2_.add(entry);
        }
        for (int i = 0; i < df3.size(); i++) {
            Entry entry = new Entry(i, df3.get(i));
            df3_.add(entry);
        }
        for (int i = 0; i < df4.size(); i++) {
            Entry entry = new Entry(i, df4.get(i));
            df4_.add(entry);
        }

        LineDataSet dataSet1 = createDataSet("df1", Color.RED, df1);
        LineDataSet dataSet2 = createDataSet("df2", Color.BLUE, df2);
        LineDataSet dataSet3 = createDataSet("df3", Color.GREEN, df3);
        LineDataSet dataSet4 = createDataSet("df4", Color.YELLOW, df4);

        YAxis leftAxis = lineChart.getAxisLeft();
        leftAxis.setEnabled(false);
        YAxis rightAxis = lineChart.getAxisRight();
        rightAxis.setEnabled(false);
        XAxis topAxis = lineChart.getXAxis();
        topAxis.setEnabled(false);

        LineData lineData = new LineData(dataSet1, dataSet2, dataSet3, dataSet4);

        lineChart.setData(lineData);

        lineChart.invalidate();
    }

    private LineDataSet createDataSet(String label, int color, List<Float> values) {
        // 将 List<Float> 轉成 List<Entry>
        List<Entry> entries = new ArrayList<>();
        for (int i = 0; i < values.size(); i++) {
            entries.add(new Entry(i, values.get(i)));
        }

        LineDataSet dataSet = new LineDataSet(entries, label);

        dataSet.setColor(color);
        dataSet.setLineWidth(2f);
        dataSet.setDrawCircles(false);
        dataSet.setMode(LineDataSet.Mode.LINEAR);

        return dataSet;
    }


    public static native int anaEcgFile(String name, String path);

    public static native int decpEcgFile(String path);
}