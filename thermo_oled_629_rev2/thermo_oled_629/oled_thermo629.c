
// OLED Thermometer.  
// By moty22.co.uk
// PIC12F629

#include <htc.h>
#include "oled_font.c"

#pragma config WDTE=OFF, MCLRE=OFF, FOSC=INTRCIO, CP=OFF, CPD=OFF    //, BOREN=OFF

#define _XTAL_FREQ 4000000
#define si GPIO2		//sensor pin
#define si_in TRISIO2=1		//sensor is input pin5
#define si_out TRISIO2=0		//sensor is output
#define SCL GPIO4 //pin3
#define SDA GPIO5 //pin2

//prototypes
void command( unsigned char comm);
void oled_init(unsigned char rows);
void clrScreen();
void sendData(unsigned char dataB);
void startBit(void);
void stopBit(void);
void clock(void);
void drawChar4(char fig, unsigned char y, unsigned char x);
unsigned char reply_in (void);
void cmnd_w_in(unsigned char cmnd);
unsigned char sensor_rst_in(void);

unsigned char addr=0x78, ty;  //0b1111000

void main(void){
    unsigned int temp;
	unsigned char d[3], minus, tempL, tempH;//  
    
    	// PIC I/O init
    CMCON = 0b111;		//comparator off
    TRISIO = 0b001101;   //SCL, SDA, trig, echo
    SCL=1;
    SDA=1;

    __delay_ms(1000);
    oled_init(64);  //number of rows in OLED

    clrScreen();       // clear screen
    drawChar4(14, ty, 93);  //deg
    drawChar4(13, ty, 110);  //C
    drawChar4(2, ty, 49);  //.
    
	while(1) {
            //read sensor
        sensor_rst_in();	//sensor init
        cmnd_w_in(0xCC);	//skip ROM command
        cmnd_w_in(0xBE);	//read pad command
        tempL=reply_in();	//LSB of temp
        tempH=reply_in();	//MSB of temp
        sensor_rst_in();
        sensor_rst_in();
        cmnd_w_in(0xCC);	//skip ROM command
        cmnd_w_in(0x44);	//start conversion command

        //tempH=0xff; tempL=0x5e;    //-10.125

        temp = tempL | ((unsigned int)tempH << 8);	//calculate temp 
        if(temp>0x7FF){temp= ~temp+1; minus=1;}else{minus=0;}	//detect negative temp
        d[2]=((temp & 15)*10)/16;		//multiply by 10 after decimal point 
        temp=temp/16;	//remove reading beyond decimal point

        d[0]=(temp/10) %10; 	//digit on left
        d[1]=temp %10;

        if(minus){drawChar4(1, ty, 0);}else{drawChar4(0, ty, 0);}	//- sign
        drawChar4(d[0]+3, ty, 17);
        drawChar4(d[1]+3, ty, 35);
        drawChar4(d[2]+3, ty, 63);

        __delay_ms(1000);
	}
	
}

void drawChar4(char fig, unsigned char y, unsigned char x)  //size Hx4 Wx3
{
    unsigned char i, j, line, b1, b2, b3, b4;    //
    
    command(0x20);    // vert mode
    command(0x01);

    command(0x21);     //col addr
    command(x); //col start
    command(x + 14);  //col end
    command(0x22);    //0x22
    command(y); // Page start
    command(y+3); // Page end
    
        startBit();
        sendData(addr);            // address
        sendData(0x40);
        
        for (i = 0; i < 5; i++){
            line=font[5*(fig)+i];
            b1=0; b2=0; b3=0; b4=0;
                // expend char    
            if(line & 64) {b4 +=240;}
            if(line & 32) {b4 +=15;}
            if(line & 16) {b3 +=240;}           
            if(line & 8) {b3 +=15;}
            
            if(line & 4) {b2 +=240;}
            if(line & 2) {b2 +=15;}
            if(line & 1) {b1 +=240;}
            for(j=0;j<3;j++){
                sendData(b1); //top page
                sendData(b2);  //second page
                sendData(b3);
                sendData(b4);
            } 
        }
        stopBit();
        
    command(0x20);      // horizontal mode
    command(0x00);    
        
}

unsigned char reply_in (void){	//reply from sensor
	unsigned char ret=0,i;
	
	for(i=0;i<8;i++){	//read 8 bits
		si_out; si=0; __delay_us(2); si_in; __delay_us(6);
		if(si){ret += 1 << i;} __delay_us(80);	//output high=bit is high
	}
	si_in;
	return ret;
}

void cmnd_w_in(unsigned char cmnd){	//send command temperature sensor
	unsigned char i;
	
	for(i=0;i<8;i++){	//8 bits 
		if(cmnd & (1 << i)){si_out; si=0; __delay_us(2); si_in; __delay_us(80);}
		else{si_out; si=0; __delay_us(80); si_in; __delay_us(2);}	//hold output low if bit is low 
	}
	si_in;
	
}

unsigned char sensor_rst_in(void){		//reset the temperature
	
	si_out;
	si=0; __delay_us(600);
	si_in; __delay_us(100);
	__delay_us(600);
	return si;	//return 0 for sensor present
	
}

void clrScreen()    //fill screen with 0
{
    unsigned char y, i;
    
    for ( y = 0; y < 8; y++ ) {
        command(0x21);     //col addr
        command(0); //col start
        command(127);  //col end
        command(0x22);    //0x22
        command(y); // Page start
        command(y+1); // Page end    
        startBit();
        sendData(addr);            // address
        sendData(0x40);
        for (i = 0; i < 128; i++){
             sendData(0x00);
        }
        stopBit();
    }    
}

//Software I2C
void sendData(unsigned char dataB)
{
    for(unsigned char b=0;b<8;b++){
       SDA=(dataB >> (7-b)) % 2;
       clock();
    }
    TRISIO5=1;   //SDA input
    clock();
    __delay_us(5);
    TRISIO5=0;   //SDA output

}

void clock(void)
{
   __delay_us(1);
   SCL=1;
   __delay_us(5);
   SCL=0;
   __delay_us(1);
}

void startBit(void)
{
    SDA=0;
    __delay_us(5);
    SCL=0;

}

void stopBit(void)
{
    SCL=1;
    __delay_us(5);
    SDA=1;

}

void command( unsigned char comm){
    
    startBit();
    sendData(addr);            // address
    sendData(0x00);
    sendData(comm);             // command code
    stopBit();
}

void oled_init(unsigned char rows) {
    if(rows==32){ty=0;}else{ty=1;}
    
    command(0xAE);   // DISPLAYOFF
    command(0x8D);         // CHARGEPUMP *
    command(0x14);     //0x14-pump on
    command(0x20);         // MEMORYMODE
    command(0x0);      //0x0=horizontal, 0x01=vertical, 0x02=page
    command(0xA1);        //SEGREMAP * A0/A1=top/bottom 
    command(0xC8);     //COMSCANDEC * C0/C8=left/right
    command(0xDA);         // SETCOMPINS *
    if(rows==32){command(0x22);}else{command(0x12);}   //0x22=4rows, 0x12=8rows
    command(0x81);        // SETCONTRAST
    command(0xFF);     //0x8F
    //next settings are set by default
//    command(0xD5);  //SETDISPLAYCLOCKDIV 
//    command(0x80);  
//    command(0xA8);       // SETMULTIPLEX
//    command(0x3F);     //0x1F
//    command(0xD3);   // SETDISPLAYOFFSET
//    command(0x0);  
//    command(0x40); // SETSTARTLINE  
//    command(0xD9);       // SETPRECHARGE
//    command(0xF1);
//    command(0xDB);      // SETVCOMDETECT
//    command(0x40);
//    command(0xA4);     // DISPLAYALLON_RESUME
 //   command(0xA6);      // NORMALDISPLAY
    command(0xAF);          //DISPLAYON

}

   
