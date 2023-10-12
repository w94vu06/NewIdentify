package com.example.newidentify;

public class identifyPlan {
    double aUpper,aLower,pUpper,pLower,cviUpper,cviLower,c1aUpper,c1aLower;

    public String First(double a, double p, double cvi, double c1a){
        aUpper = a+a*0.2;
        aLower = a-a*0.2;
        pUpper = p+p*0.25;
        pLower = p-p*0.25;
        cviUpper = cvi+cvi*0.2;
        cviLower = cvi-cvi*0.2;
        c1aUpper = c1a+c1a*0.25;
        c1aLower = c1a-c1a*0.25;
        if (a>=aLower && a<=aUpper ){
            if (p>=pLower && p<=pUpper){
                if (cvi>=cviLower && cvi<=cviUpper){
                    if (c1a>=c1aLower && c1a<=c1aUpper){
                        return "本人";
                    }else {
                        return "非本人";
                    }
                }else {
                    return "非本人";
                }
            }else {
                return "非本人";
            }
        }else {
            return "非本人";
        }
    }

    public String Second(double a, double p, double cvi, double c1a){
        aUpper = a+a*0.25;
        aLower = a-a*0.25;
        pUpper = p+p*0.3;
        pLower = p-p*0.3;
        cviUpper = cvi+cvi*0.4;
        cviLower = cvi-cvi*0.4;
        c1aUpper = c1a+c1a*0.3;
        c1aLower = c1a-c1a*0.3;
        if (a>=aLower && a<=aUpper ){
            if (p>=pLower && p<=pUpper){
                if (cvi>=cviLower && cvi<=cviUpper){
                    if (c1a>=c1aLower && c1a<=c1aUpper){
                        return "本人";
                    }else {
                        return "非本人";
                    }
                }else {
                    return "非本人";
                }
            }else {
                return "非本人";
            }
        }else {
            return "非本人";
        }
    }
}
