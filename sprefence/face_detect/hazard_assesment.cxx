#include "hazard_assesment.h"
#include <stdio.h>
#include <string>

const std::string hazardSettingPath = "/mnt/sd0/CONFIG/hazard.ini";

static double hazardLT = 0.25;
static double hazardRT = 0.75;
static double hazardLB = 0.25;
static double hazardRB = 0.75;

int LoadSetting(){
    FILE* fp = fopen(hazardSettingPath.c_str(), "r");
    if(fp == NULL){
        return -1;
    }

    fscanf(fp, "%lf %lf %lf %lf", &hazardLT, &hazardRT, &hazardLB, &hazardRB);

    fclose(fp);
    return 0;
}

bool IsHazard(float x, float y){
    double dL = hazardLT - hazardLB;
    double lX = 0.0;
    if (dL > -0.001 && dL < 0.001){
        lX = hazardLB;
    }else{
        double gradL = 1.0 / dL;
        lX = hazardLB + gradL * y;
    }

    if (x < lX){
        return true;
    }

    double dR = hazardRT - hazardRB;
    double rX = 0.0;

    if (dR > -0.001 && dR < 0.001){
        rX = hazardRB;
    }else{
        double gradR = 1.0 / dR;
        rX = hazardRB + gradR * y;
    }

    if (rX < x){
        return true;
    }

    return false;
}