package com.example.newidentify.processData;

import android.util.Log;

import com.github.mikephil.charting.data.Entry;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class SignalProcess extends Thread{
    private static final String TAG = "SignalProcess";

    public List<Float> getReduceRR100(List<Float> dataList, int startIndex, int endIndex) {
        List<Entry> dataBetweenTwoR = new ArrayList<>();

        startIndex = Math.max(0, startIndex);
        endIndex = Math.min(dataList.size() - 1, endIndex);

        for (int i = startIndex; i <= endIndex; i++) {
            int xOffset = i - startIndex;
            dataBetweenTwoR.add(new Entry(xOffset, dataList.get(i)));
        }

        return reduceSampling(dataBetweenTwoR);
    }

    private List<Float> reduceSampling(List<Entry> input) {
        int originalLength = input.size();
        int targetLength = Math.max(originalLength / 100, 100);

        List<Float> result = new ArrayList<>();
        int step = originalLength / targetLength;
        for (int i = 0; i < originalLength; i++) {
            float sum = 0;
            int count = 0;

            for (float j = i * step; j < (i + 1) * step && j < originalLength; j++) {
                sum += input.get((int) j).getY();
                count++;
            }

            if (count > 0) {
                result.add(sum / count);
            }
        }
        return get50Point(result);
    }

    private List<Float> get50Point(List<Float> result) {
        int midPoint = result.size() / 2;
        int startIndex = midPoint - 50;
        int endIndex = midPoint + 50;

        // 確保 startIndex 和 endIndex 在有效範圍內
        startIndex = Math.max(0, startIndex);
        endIndex = Math.min(result.size() - 1, endIndex);

        List<Float> result2 = new ArrayList<>();
        for (int i = startIndex; i <= endIndex; i++) {
            result2.add(result.get(i));
        }
        if (result2.size() > 100) {
            result2 = result2.subList(0, 100);
        } else if (result2.size() < 100) {
            while (result2.size() < 100) {
                // add a average value
                float sum = 0;
                for (float value : result2) {
                    sum += value;
                }
                result2.add(sum / result2.size());
            }
        }
        return result2;
    }

    // 計算兩個列表的中位數差異
    public float calMidDiff(List<Float> data1, List<Float> data2) {
        // 檢查兩個列表的大小是否相同，如果不同，可能需要進行錯誤處理
        if (data1.size() != data2.size()) {
            return 0;
        }

        // 創建存儲中位數差異的列表
        List<Float> differences = new ArrayList<>();

        // 計算差異
        for (int i = 0; i < data1.size(); i++) {
            float diff = (data2.get(i) - data1.get(i)) / data1.get(i);
            differences.add(diff);
        }

        // 計算中位數
        float midDiff = calMid(differences);

        return midDiff;
    }

    public float calMid(List<Float> values) {
        // 對值進行排序
        Collections.sort(values);

        int size = values.size();

        // 計算中位數
        if (size % 2 == 0) {
            // 如果列表大小為偶數，取中間兩個值的平均
            int middleIndex1 = size / 2 - 1;
            int middleIndex2 = size / 2;
            return (values.get(middleIndex1) + values.get(middleIndex2)) / 2.0f;
        } else {
            // 如果列表大小為奇數，取中間值
            int middleIndex = size / 2;
            return values.get(middleIndex);
        }
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
            Log.d("xxxxx", "exexexex = " + ex);
            return indata.get(indata.size() - 1);
        }
    }
}
