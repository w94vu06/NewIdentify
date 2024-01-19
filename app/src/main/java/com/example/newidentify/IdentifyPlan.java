package com.example.newidentify;

import android.util.Log;

public class IdentifyPlan {
    double aUpper,aLower,pUpper,pLower,cviUpper,cviLower,c1aUpper,c1aLower;

    public String First(double averageHR, double HR, double averagePI, double PI, double averageCvi, double Cvi, double averageC1a, double C1a){
        aUpper = averageHR*1.2;
        aLower = averageHR*0.8;
        pUpper = averagePI*1.25;
        pLower = averagePI*0.75;
        cviUpper = averageCvi*1.35;
        cviLower = averageCvi*0.65;
        c1aUpper = averageC1a*1.25;
        c1aLower = averageC1a*0.75;
        if (HR>=aLower && HR<=aUpper && PI>=pLower && PI<=pUpper && Cvi>=cviLower && Cvi<=cviUpper && C1a>=c1aLower && C1a<=c1aUpper ){
            String s = String.format("Second\nhrup: %f hrlow: %f\npup: %f plow: %f\ncviup: %f cvilow: %f\nc1aup: %f c1alow: %f",
                    aUpper,aLower,pUpper,pLower,cviUpper,cviLower,c1aUpper,c1aLower);
            Log.d("ans",s);
            return "本人";
        }else {
            return "非本人";
        }
    }

    public String Second(double averageHR, double HR, double averagePI, double PI, double averageCvi, double Cvi, double averageC1a, double C1a){
        aUpper = averageHR*1.25;
        aLower = averageHR*0.75;
        pUpper = averagePI*1.3;
        pLower = averagePI*0.7;
        cviUpper = averageCvi*1.4;
        cviLower = averageCvi*0.6;
        c1aUpper = averageC1a*1.3;
        c1aLower = averageC1a*0.7;
        if (HR>=aLower && HR<=aUpper && PI>=pLower && PI<=pUpper && Cvi>=cviLower && Cvi<=cviUpper && C1a>=c1aLower && C1a<=c1aUpper ){
            String s = String.format("Second\nhrup: %f hrlow: %f\npup: %f plow: %f\ncviup: %f cvilow: %f\nc1aup: %f c1alow: %f",
                aUpper,aLower,pUpper,pLower,cviUpper,cviLower,c1aUpper,c1aLower);
            Log.d("ans",s);
            return "本人";
        }else {
            return "非本人";
        }
    }

}
