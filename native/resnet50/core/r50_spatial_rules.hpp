#pragma once
#include "resnet50/engine/polyphase_convolution.hpp"
struct PublicSpatial {unsigned ell,row,k,channels,unit;};
inline PublicSpatial r50_spatial(unsigned id,unsigned oi){if(id==0&&aqcf_polyphase458::enabled()){if(oi>=128)throw std::runtime_error("458 output range");return {8192,115,4,1,1};}switch(id){
case 0:
if(oi<22)return {2553,69,7,3,2553};
if(oi<44)return {2553,69,7,3,2553};
if(oi<66)return {2553,69,7,3,2553};
if(oi<79)return {1369,37,7,5,1};
if(oi<101)return {2553,69,7,3,2553};
if(oi<123)return {2553,69,7,3,2553};
if(oi<145)return {2553,69,7,3,2553};
if(oi<158)return {1369,37,7,5,1};
if(oi<180)return {2553,69,7,3,2553};
if(oi<202)return {2553,69,7,3,2553};
if(oi<224)return {2553,69,7,3,2553};
if(oi<237)return {1369,37,7,5,1};
if(oi<259)return {2553,69,7,3,2553};
if(oi<281)return {2553,69,7,3,2553};
if(oi<303)return {2553,69,7,3,2553};
if(oi<316)return {1369,37,7,5,1};
if(oi<338)return {2553,69,7,3,2553};
if(oi<360)return {2553,69,7,3,2553};
if(oi<382)return {2553,69,7,3,2553};
if(oi<395)return {1369,37,7,5,1};
if(oi<417)return {2553,69,7,3,2553};
if(oi<439)return {2553,69,7,3,2553};
if(oi<461)return {2553,69,7,3,2553};
if(oi<474)return {1369,37,7,5,1};
if(oi<496)return {2553,69,7,3,2553};
if(oi<518)return {2553,69,7,3,2553};
if(oi<540)return {2553,69,7,3,2553};
if(oi<553)return {1369,37,7,5,1};
throw std::runtime_error("public spatial tile index");
case 4:
if(oi<32)return {3249,57,3,2,11785};
throw std::runtime_error("public spatial tile index");
case 8:
if(oi<32)return {3249,57,3,2,11785};
throw std::runtime_error("public spatial tile index");
case 12:
if(oi<32)return {3249,57,3,2,11785};
throw std::runtime_error("public spatial tile index");
case 17:
if(oi<64)return {3249,57,3,2,11785};
throw std::runtime_error("public spatial tile index");
case 21:
if(oi<43)return {841,29,3,9,7};
throw std::runtime_error("public spatial tile index");
case 26:
if(oi<43)return {841,29,3,9,7};
throw std::runtime_error("public spatial tile index");
case 30:
if(oi<43)return {841,29,3,9,7};
throw std::runtime_error("public spatial tile index");
case 35:
if(oi<86)return {841,29,3,9,7};
throw std::runtime_error("public spatial tile index");
case 39:
if(oi<29)return {225,15,3,36,801};
throw std::runtime_error("public spatial tile index");
case 43:
if(oi<29)return {225,15,3,36,801};
throw std::runtime_error("public spatial tile index");
case 47:
if(oi<29)return {225,15,3,36,801};
throw std::runtime_error("public spatial tile index");
case 51:
if(oi<29)return {225,15,3,36,801};
throw std::runtime_error("public spatial tile index");
case 55:
if(oi<29)return {225,15,3,36,801};
throw std::runtime_error("public spatial tile index");
case 60:
if(oi<57)return {225,15,3,36,801};
throw std::runtime_error("public spatial tile index");
case 64:
if(oi<32)return {64,8,3,128,1};
throw std::runtime_error("public spatial tile index");
case 68:
if(oi<32)return {64,8,3,128,1};
throw std::runtime_error("public spatial tile index");
default:return {0,0,0,0,0};}}
