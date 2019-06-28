rm armusb_v1.0
echo "rm armusb_v1.0 success"

arm-linux-gnueabihf-gcc  usb.c Funtion.c -o armusb_v1.0 ../scpi-def.c  -I ../ -lm ../scpi/libscpi.a
echo "compile armusb_v1.0 sucess"