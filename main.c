#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "lcd.h"

bulletColour = RED;
playerColour = BLUE;
turretColour = 0x8410;

struct bullet{
    char direction;
    int16_t xPos, oldXPos;
    int16_t yPos, oldYPos;
};

volatile uint16_t playerY = 150;
volatile uint16_t playerX = 100; /*player starting x and y pos*/
#define playerSize 17
volatile uint8_t turretPos = 0; /*starting turret direction (up)*/
volatile rectangle turrRect;
#define maxBullets 3
volatile uint8_t currentBullet = 0;
volatile struct bullet bullets[maxBullets] = {{'-', 0, 0, 0, 0}, {'-', 0, 0, 0, 0}, {'-', 0, 0, 0, 0}};
uint8_t redraw =1;
volatile uint16_t delta;


/*function definitions*/

void movePlayer(void);
int8_t enc_delta(void);
void moveTurret(uint8_t x);
void createBullet(uint8_t x);
void moveBullets(void);



void init(){
    CLKPR = (1 << CLKPCE);
    CLKPR = 0;
    DDRB = _BV(PB7); /*set port b pin 7 to output*/

    PORTE=_BV(PE7) | _BV(PE5) | _BV(PE4); /*pull up port e pins 7,5,4 (switch, rotb, rota)*/

    PORTC = _BV(PC5) | _BV(PC4) | _BV(PC3) | _BV(PC2); /*pull up port c pins 5,4,3,2 (w,s,e,n buttons)*/
    
    EIMSK = _BV(PE7) | /*_BV(PE4) | */_BV(INT6); /*enable external interrupt mask bit for pin e 7 (switch)*/
    EICRB = _BV(ISC70) | /*_BV(ISC40) | */_BV(ISC60); /*enable external level interrupt pin e 7 (switch)*/
    

    TCCR0A = _BV(WGM01); /*set to clear timer on match mode?*/
    TCCR0B = 0x03; /*enable clock with clkIO/64 prescaling setting*/
    OCR0A = 0x7C; /*set timer a compare value to 124 (when timer hits 124, interrupt is fired)*/
    TIMSK0 =  _BV(OCIE0A); /*enable timer interrupt (0x02 or 0b00000010)*/
 
    
}



/*screen interrupt / main game loop*/
ISR(INT6_vect){
    int8_t val = turretPos + enc_delta();
    if(val<0) val=3;
    if(val>3) val=0;
    turretPos=val;
    

    movePlayer();

    moveBullets();

}

/*switch interrupt for creating bullets*/
ISR(INT7_vect){
    createBullet(turretPos);
}


void main(){
    
    init();
    sei();
    init_lcd();
    /*set_frame_rate_hz(61);*/
    while (1){
        
    }

}

/*function scans arrow inputs and moves player pos accordingly*/
void movePlayer(){
    /*board will be held sideways so north arrow = left movement. */
    uint16_t prevX = playerX;
    uint16_t prevY = playerY;

    if(! (PINC & _BV(PC2))){ /*left pressed*/
        if(playerX>10){
            playerX-=1;
            redraw=1;
        }
        
    }

    if(! (PINC & _BV(PC4))){ /*right pressed*/
        if(playerX<210){
            playerX+=1;
            redraw=1;
        }
        
    }

    if(! (PINC & _BV(PC3))){ /*up pressed*/
        if(playerY>10){
            playerY-=1;
            redraw=1;
        }
        
    }

    if(! (PINC & _BV(PC5))){ /*down pressed*/
        if(playerY<290){
            playerY+=1;
            redraw=1;
        }
        
    }
    moveTurret(turretPos);
    
    if(redraw){
        fill_rectangle((rectangle){prevX, prevX+playerSize,prevY,prevY+playerSize},BLACK);
        fill_rectangle((rectangle){playerX, playerX+playerSize, playerY, playerY+playerSize}, playerColour);
        fill_rectangle(turrRect, turretColour);

        redraw=0;
    }
    _delay_ms(10);
}

/*Draws turret in correct orientation*/
void moveTurret(uint8_t turretPos){
        switch(turretPos){
        case 0: /*turret facing up*/
            turrRect.left = playerX + playerSize/2 +1 -2;
            turrRect.right = playerX + playerSize/2 +1 +2;
            turrRect.top = playerY +2; 
            turrRect.bottom = playerY + playerSize/2 +1;
            break;
        case 1: /*turret facing left*/
            turrRect.left = playerX + 2;
            turrRect.right = playerX + playerSize/2 +1;
            turrRect.top = playerY + playerSize/2 +1 -2;
            turrRect.bottom = playerY + playerSize/2 +1 +2;
            break;
        case 2: /*turret facing down*/
            turrRect.left = playerX + playerSize/2 +1 -2;
            turrRect.right = playerX + playerSize/2 +1 +2;
            turrRect.top = playerY + playerSize/2 +1;
            turrRect.bottom = playerY + playerSize -2;
            break;
        case 3: /*turret facing right*/
            turrRect.left = playerX + playerSize/2 +1;
            turrRect.right = playerX + playerSize -2;
            turrRect.top = playerY + playerSize/2 +1 -2;
            turrRect.bottom = playerY + playerSize/2 +1 +2;
            break;
    }
    redraw=1;

}

/*if switch pressed create bullet*/
void createBullet(uint8_t turretPos){
    if(! (PINE & _BV(PE7)) && bullets[currentBullet].direction=='-'){ /*if switch pressed AND current bullet slot not in use*/
        
        
        char dir;
        uint8_t xPos;
        uint16_t yPos;
        switch (turretPos)
        {
            case 0:
                dir = 'U';
                xPos=turrRect.left + 1;
                yPos=turrRect.top - 3;
                break;
            case 1:
                dir = 'L';
                xPos=turrRect.left - 3;
                yPos=turrRect.top + 1;
                break;
            case 2:
                dir = 'D';
                xPos=turrRect.left + 1;
                yPos=turrRect.bottom + 3;
                break;
            case 3:
                dir = 'R';
                xPos=turrRect.right + 3;
                yPos=turrRect.top + 1;
                break;
        }
        bullets[currentBullet].direction=dir; 
        bullets[currentBullet].xPos = xPos;
        bullets[currentBullet].oldXPos = xPos;
        bullets[currentBullet].yPos = yPos;
        bullets[currentBullet].oldYPos = yPos;
        currentBullet++;
        if(currentBullet>=maxBullets) currentBullet=0;
    }
}

/*move and draw bullets*/
void moveBullets(){ 
    int i=0;
    for(i; i<maxBullets; i++){
        if(bullets[i].direction!='-'){
            bullets[i].oldXPos=bullets[i].xPos;
            bullets[i].oldYPos=bullets[i].yPos;
            switch (bullets[i].direction)
            {
                case 'U':
                    bullets[i].yPos-=2;
                    if(bullets[i].yPos+3<=0){
                        bullets[i].direction='-';
                        rectangle rect = {bullets[i].xPos, bullets[i].xPos+3, bullets[i].yPos, bullets[i].yPos+3};
                        fill_rectangle(rect, BLACK);
                    }
                    break;
                case 'L':
                    bullets[i].xPos-=2;
                    if(bullets[i].xPos+3<=0){
                        bullets[i].direction='-';
                        rectangle rect = {bullets[i].xPos, bullets[i].xPos+3, bullets[i].yPos, bullets[i].yPos+3};
                        fill_rectangle(rect, BLACK);
                    }
                    break;
                case 'D':
                    bullets[i].yPos+=2;
                    if(bullets[i].yPos>=320){
                        bullets[i].direction='-';
                        rectangle rect = {bullets[i].xPos, bullets[i].xPos+3, bullets[i].yPos, bullets[i].yPos+3};
                        fill_rectangle(rect, BLACK);
                    }
                    break;
                case 'R':
                    bullets[i].xPos+=2;
                    if(bullets[i].xPos>=240){
                        bullets[i].direction='-';
                        rectangle rect = {bullets[i].xPos, bullets[i].xPos+3, bullets[i].yPos, bullets[i].yPos+3};
                        fill_rectangle(rect, BLACK);
                    }
                    break;
            }
            rectangle currentBulletPos = {bullets[i].xPos, bullets[i].xPos+3, bullets[i].yPos, bullets[i].yPos+3};
            rectangle oldBulletPos = {bullets[i].oldXPos, bullets[i].oldXPos+3, bullets[i].oldYPos, bullets[i].oldYPos+3};
            
            fill_rectangle(oldBulletPos, BLACK);
            if(bullets[i].direction!='-') fill_rectangle(currentBulletPos, bulletColour);
        }
    }
}

/*following ISR and enc_delta taken from task 1 skeleton code*/
 ISR( TIMER0_COMPA_vect ) {
     static int8_t last;
     int8_t new, diff;
     uint8_t wheel;


     /*
        Scan rotary encoder
        ===================
        This is adapted from Peter Dannegger's code available at:
        http://www.mikrocontroller.net/articles/Drehgeber
     */

     wheel = PINE;
     new = 0;
     if( wheel  & _BV(PE4) ) new = 3;
     if( wheel  & _BV(PE5) ) new ^= 1;		       	/* convert gray to binary */
     diff = last - new;
     if( diff & 1 ){			/* bit 0 = value (1) */
	     last = new;
	     delta += (diff & 2) - 1;	/* bit 1 = direction (+/-) */
     }

}


/* read two step encoder */
int8_t enc_delta() {
    int8_t val;

    cli();
    val = delta;
    delta &= 1;
    sei();

    return val >> 1;
}