#pragma once
inline unsigned r50_public_tile_spacing(unsigned id,unsigned oi){switch(id){
case 2:
if(oi<64)return 256;
if(oi<107)return 64;
throw std::runtime_error("public output tile range");
case 3:
if(oi<32)return 64;
throw std::runtime_error("public output tile range");
case 5:
if(oi<64)return 256;
if(oi<107)return 64;
throw std::runtime_error("public output tile range");
case 7:
if(oi<32)return 256;
if(oi<64)return 64;
throw std::runtime_error("public output tile range");
case 9:
if(oi<64)return 256;
if(oi<107)return 64;
throw std::runtime_error("public output tile range");
case 11:
if(oi<32)return 256;
if(oi<64)return 64;
throw std::runtime_error("public output tile range");
case 13:
if(oi<64)return 256;
if(oi<107)return 64;
throw std::runtime_error("public output tile range");
case 15:
if(oi<103)return 16;
throw std::runtime_error("public output tile range");
case 16:
if(oi<64)return 256;
if(oi<107)return 64;
throw std::runtime_error("public output tile range");
case 18:
if(oi<52)return 16;
throw std::runtime_error("public output tile range");
case 20:
if(oi<64)return 16;
throw std::runtime_error("public output tile range");
case 23:
if(oi<52)return 16;
throw std::runtime_error("public output tile range");
case 25:
if(oi<64)return 16;
throw std::runtime_error("public output tile range");
case 27:
if(oi<52)return 16;
throw std::runtime_error("public output tile range");
case 29:
if(oi<64)return 16;
throw std::runtime_error("public output tile range");
case 31:
if(oi<52)return 16;
throw std::runtime_error("public output tile range");
case 33:
if(oi<103)return 4;
throw std::runtime_error("public output tile range");
case 34:
if(oi<52)return 16;
throw std::runtime_error("public output tile range");
case 36:
if(oi<52)return 4;
throw std::runtime_error("public output tile range");
case 38:
if(oi<64)return 4;
throw std::runtime_error("public output tile range");
case 40:
if(oi<52)return 4;
throw std::runtime_error("public output tile range");
case 42:
if(oi<64)return 4;
throw std::runtime_error("public output tile range");
case 44:
if(oi<52)return 4;
throw std::runtime_error("public output tile range");
case 46:
if(oi<64)return 4;
throw std::runtime_error("public output tile range");
case 48:
if(oi<52)return 4;
throw std::runtime_error("public output tile range");
case 50:
if(oi<64)return 4;
throw std::runtime_error("public output tile range");
case 52:
if(oi<52)return 4;
throw std::runtime_error("public output tile range");
case 54:
if(oi<64)return 4;
throw std::runtime_error("public output tile range");
case 56:
if(oi<52)return 4;
throw std::runtime_error("public output tile range");
case 58:
if(oi<76)return 1;
throw std::runtime_error("public output tile range");
case 59:
if(oi<64)return 4;
throw std::runtime_error("public output tile range");
case 61:
if(oi<50)return 1;
throw std::runtime_error("public output tile range");
case 63:
if(oi<47)return 1;
throw std::runtime_error("public output tile range");
case 65:
if(oi<50)return 1;
throw std::runtime_error("public output tile range");
case 67:
if(oi<47)return 1;
throw std::runtime_error("public output tile range");
case 69:
if(oi<50)return 1;
throw std::runtime_error("public output tile range");
case 71:
if(oi<11)return 1;
throw std::runtime_error("public output tile range");
default:return 1;}}
