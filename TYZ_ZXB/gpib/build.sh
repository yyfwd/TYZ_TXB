rm armgpib_v1.0
echo "rm armgpib_v1.0 success"

arm-linux-gnueabihf-gcc   gpib.c  gpmc.c  NGPIB_IO.c  Funtion.c  -g3  -o armgpib_v1.0  ../scpi-def.c  -I ../ -lm ../scpi/libscpi.a -lpthread
echo "compile armgpib_v1.0 sucess"