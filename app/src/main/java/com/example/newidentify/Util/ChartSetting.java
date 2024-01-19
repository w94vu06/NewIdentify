package com.example.newidentify.Util;

import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.data.LineData;

public class ChartSetting {

    public void initchart(LineChart lineChart) {
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

        lineChart.invalidate();
    }
}
