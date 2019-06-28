rm armvxi11_v1.0 
echo "rm armvxi11_v1.0 success"

arm-linux-gnueabihf-gcc  -o armvxi11_v1.0  vxi11_xdr.c vxi11_srv.c vxi11_clnt.c main.c uart_srv.c data_send_save.c init_srv.c scpi-def.c udpuart.c -I ./ -lm ./scpi/libscpi.a
echo "compile armvxi11_v1.0 sucess"
