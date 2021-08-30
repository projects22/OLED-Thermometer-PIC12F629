
// OLED Thermometer.  
// By moty22.co.uk
// PIC12F629

#include <htc.h>
#include "oled_font.c"

#pragma config WDTE=OFF, MCLRE=OFF, BOREN=OFF, FOSC=INTRCIO, CP=OFF, CPD=OFF    //

#define _XTAL_FREQ 4000000
#define si GPIO2		//sensor pin
#define si_in TRISIO2=1		//sensor is input pin5
#define si_out TRISIO2=0		//sensor is output
#define SCL GPIO4 //pin3
#define SDA GPIO5 //pin2

//prototypes
void command( unsigned char comm);
void oled_init();
void clrScreen();
void sendData(unsigned char dataB);
void startBit(void);
void stopBit(void);
void clock(void);
void drawChar2(char fig, unsigned char y, unsigned char x);
unsigned char reply_in (void);
void cmnd_w_in(unsigned char cmnd);
unsigned char sensor_rst_in(void);

unsigned char addr=0x78;  //0b1111000

void main(void){
    unsigned int temp;
	unsigned char d[3], minus, tempL, tempH;//  
    
    	// PIC I/O init
    CMCON = 0b111;		//comparator off
    TRISIO = 0b001101;   //SCL, SDA, trig, echo
    SCL=1;
    SDA=1;

    __delay_ms(1000);
    oled_init();

    clrScreen();       // clear screen
    drawChar2(14, 1, 7);  //deg
    drawChar2(13, 1, 8);  //C
    drawChar2(2, 1, 4);  //.
    
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

        //temp=0xFFF8;    //-0.5
        //temp=0xFF5E;    //-10.125
        //temp=0xFC90;    //-55

        temp = tempL | ((unsigned int)tempH << 8);	//calculate temp 
        if(temp>0x7FF){temp= ~temp+1; minus=1;}else{minus=0;}	//detect negative temp
        d[2]=((temp & 15)*10)/16;		//multiply by 10 after decimal point 
        temp=temp/16;	//remove reading beyond decimal point

        d[0]=(temp/10) %10; 	//digit on left
        d[1]=temp %10;

        if(minus){drawChar2(1, 1, 1);}else{drawChar2(0, 1, 1);}	//- sign
        drawChar2(d[0]+3, 1, 2); 
        drawChar2(d[1]+3, 1, 3);
        drawChar2(d[2]+3, 1, 5);

        __delay_ms(1000);
	}
	
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

   //size 2 chars
void drawChar2(char fig, unsigned char y, unsigned char x)
{
    unsigned char i, line, btm, top;    //
    
    command(0x20);    // vert mode
    command(0x01);

    command(0x21);     //col addr
    command(13 * x); //col start
    command(13 * x + 9);  //col end
    command(0x22);    //0x22
    command(y); // Page start
    command(y+1); // Page end
    
    startBit();
    sendData(addr);            // address
    sendData(0x40);

    for (i = 0; i < 5; i++){
        line=font[5*(fig)+i];
        btm=0; top=0;
            // expend char    
        if(line & 64) {btm +=192;}
        if(line & 32) {btm +=48;}
        if(line & 16) {btm +=12;}           
        if(line & 8) {btm +=3;}

        if(line & 4) {top +=192;}
        if(line & 2) {top +=48;}
        if(line & 1) {top +=12;}        

         sendData(top); //top page
         sendData(btm);  //second page
         sendData(top);
         sendData(btm);
    }
    stopBit();
        
    command(0x20);      // horizontal mode
    command(0x00);    
        
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

void oled_init() {
    
    command(0xAE);   // DISPLAYOFF
    command(0x8D);         // CHARGEPUMP *
    command(0x14);     //0x14-pump on
    command(0x20);         // MEMORYMODE
    command(0x0);      //0x0=horizontal, 0x01=vertical, 0x02=page
    command(0xA1);        //SEGREMAP * A0/A1=top/bottom 
    command(0xC8);     //COMSCANDEC * C0/C8=left/right
    command(0xDA);         // SETCOMPINS *
    command(0x22);   //0x22=4rows, 0x12=8rows
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

   
