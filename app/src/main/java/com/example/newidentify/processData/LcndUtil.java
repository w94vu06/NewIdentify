package com.example.newidentify.processData;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

public class LcndUtil {
    public static int len (String path, char[] a){
        File f = new File(path);
        try {
            FileInputStream fs = new FileInputStream(f);
            DataInputStream in = new DataInputStream(fs);
            InputStreamReader isr = new InputStreamReader(in, StandardCharsets.ISO_8859_1);
            BufferedReader br = new BufferedReader(isr);
            int length;
            length = br.read(a,0,32*1024*1024);
            br.close();
            isr.close();
            in.close();
            fs.close();
            return length;
        }catch (IOException e){
            e.printStackTrace();
        }
        return 0;
    }

    public static byte[] readFromByteFile(String pathname){
        File filename = new File(pathname);
        try{
            BufferedInputStream in = new BufferedInputStream(new FileInputStream(filename));
            ByteArrayOutputStream out = new ByteArrayOutputStream(1024);
            byte[] temp = new byte[1024];
            int size = 0;
            while((size = in.read(temp)) != -1){
                out.write(temp, 0, size);
            }
            in.close();
            byte[] content = out.toByteArray();
            return content;
        }catch (IOException e){
            e.printStackTrace();
        }
        byte[] a = new byte[1024];
        return a;
    }
}
