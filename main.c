#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include "lcd.h"

bulletColour = RED;
playerColour = BLUE;
turretColour = 0x8410;

struct object{
    char direction;
    int16_t xPos, oldXPos;
    int16_t yPos, oldYPos;
};

volatile uint16_t playerY = 150;
volatile uint16_t playerX = 100; /*player starting x and y pos*/
#define playerSize 17
volatile uint8_t turretPos = 0; /*starting turret direction (up)*/
volatile rectangle turrRect;
volatile uint8_t maxBullets = 1;
volatile uint8_t currentBullet = 0;
volatile struct object bullets[3] = {{'-', 0, 0, 0, 0}, {'-', 0, 0, 0, 0}, {'-', 0, 0, 0, 0}};
uint8_t redraw =1;
volatile uint16_t delta;

#define baseEnemySpeed 2
#define trackingEnemySpeed 1
volatile uint8_t score=0;
volatile uint8_t lives=3;
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
    

    TCCR0A = _BV(WGM01); /*set to clear timer on match mode*/
    TCCR0B = 0x03; /*enable clock with clkIO/64 prescaling setting*/
    OCR0A = 0x7C; /*set timer a compare value to 124 (when timer hits 124, interrupt is fired)*/
    TIMSK0 =  _BV(OCIE0A); /*enable timer interrupt (0x02 or 0b00000010)*/
 
    
}

uint8_t timerMatch=100;
/*screen interrupt / main game loop*/
ISR(INT6_vect){
    int8_t val = turretPos + enc_delta();
    if(val<0) val=3;
    if(val>3) val=0;
    turretPos=val;
    
    movePlayer();

    moveBullets();

    if(timerMatch){
        timerMatch--;
    }else{
        spawnEnemy();
        moveEnemies();
        timerMatch=4;
    }
    detectCollisions();

    char buffer[4];
    sprintf(buffer, "%03d",score);
    display_string_xy(buffer,7,311);

    char buffer2[2];
    sprintf(buffer2, "%02d",lives);
    display_string_xy(buffer2,227,311);
    
}

/*switch interrupt for creating bullets*/
ISR(INT7_vect){
    createBullet(turretPos);
}


void main(){
    
    init();
    init_lcd();
    /*Start menu goes here*/
    display_string("Start menu test.");
    display_string_xy("Press switch to start.",40,160);
    while(PINE & _BV(PE7)){
    }
    srand(TCNT0); /*seeding rng with counter value (happens when user presses button to start so slightly more random)*/
    clear_screen();
    _delay_ms(500);
    sei();

    /*set_frame_rate_hz(61);*/

    /*loop keep game running*/
    /*TODO replace 1 with live system*/
    
    while (lives){
        
    }
    cli();
    clear_screen();
    display_string_xy("You died!", 100, 155);
    display_string_xy("Press switch to restart. NYI", 60,165); 


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

uint8_t enemiesOnScreen=0;
#define maxEnemies 3
#define enemySize 11

volatile struct object enemies[maxEnemies] = {{'-', 0, 0, 0, 0},{'-', 0, 0, 0, 0},{'-', 0, 0, 0, 0}};

/*Spawn new enemy if there are fewer than max enemies on screen atm*/
void spawnEnemy(){
    uint8_t currentEnemy=0;
    if(enemiesOnScreen<maxEnemies){
        for(currentEnemy; currentEnemy<=maxEnemies; currentEnemy++){
            if(enemies[currentEnemy].direction=='-'){ 
                char dir;
                uint8_t xPos;
                uint16_t yPos;
                uint8_t enemyType = rand() % 5;
                switch (enemyType)
                {
                    case 0:
                        dir = 'R';
                        xPos=0;
                        yPos=(rand() % 280) +10;
                        break;
                    case 1:
                        dir = 'L';
                        xPos=230;
                        yPos=(rand() % 280) +10;
                        break;
                    case 2:
                        dir = 'U';
                        xPos=(rand() % 210) +10;
                        yPos=290;
                        break;
                    case 3:
                        dir = 'D';
                        xPos=(rand() % 210) +10;
                        yPos=0;
                        break;
                    case 4:
                        dir = 'F';
                        uint8_t side = rand() % 4;
                        switch(side){
                            case 0: /*come from left*/
                                xPos=0;
                                yPos=(rand() % 280) +10;
                                break;
                            case 1: /*come from right*/
                                xPos=230;
                                yPos=(rand() % 280) +10;
                                break;
                            case 2: /*come from bottom*/
                                xPos=(rand() % 210) +10;
                                yPos=290;
                                break;
                            case 3: /*come from top*/
                                xPos=(rand() % 210) +10;
                                yPos=0;
                                break;
                        }
                        break;
                }
                enemies[currentEnemy].direction=dir; 
                enemies[currentEnemy].xPos = xPos;
                enemies[currentEnemy].oldXPos = xPos;
                enemies[currentEnemy].yPos = yPos;
                enemies[currentEnemy].oldYPos = yPos;
                enemiesOnScreen++;
            }
        }
    }
}


void moveEnemies(){ 
    int i=0;
    for(i; i<maxEnemies; i++){
        if(enemies[i].direction!='-'){
            enemies[i].oldXPos=enemies[i].xPos;
            enemies[i].oldYPos=enemies[i].yPos;
            switch (enemies[i].direction)
            {
                case 'U':
                    enemies[i].yPos-=baseEnemySpeed;
                    if(enemies[i].yPos+enemySize<=0){
                        enemies[i].direction='-';
                        rectangle rect = {enemies[i].xPos, enemies[i].xPos+enemySize, enemies[i].yPos, enemies[i].yPos+enemySize};
                        fill_rectangle(rect, BLACK);
                        enemiesOnScreen--;
                    }
                    break;
                case 'L':
                    enemies[i].xPos-=baseEnemySpeed;
                    if(enemies[i].xPos+enemySize<=0){
                        enemies[i].direction='-';
                        rectangle rect = {enemies[i].xPos, enemies[i].xPos+enemySize, enemies[i].yPos, enemies[i].yPos+enemySize};
                        fill_rectangle(rect, BLACK);
                        enemiesOnScreen--;
                    }
                    break;
                case 'D':
                    enemies[i].yPos+=baseEnemySpeed;
                    if(enemies[i].yPos>=320){
                        enemies[i].direction='-';
                        rectangle rect = {enemies[i].xPos, enemies[i].xPos+enemySize, enemies[i].yPos, enemies[i].yPos+enemySize};
                        fill_rectangle(rect, BLACK);
                        enemiesOnScreen--;
                    }
                    break;
                case 'R':
                    enemies[i].xPos+=2;
                    if(enemies[i].xPos>=240){
                        enemies[i].direction='-';
                        rectangle rect = {enemies[i].xPos, enemies[i].xPos+enemySize, enemies[i].yPos, enemies[i].yPos+enemySize};
                        fill_rectangle(rect, BLACK);
                        enemiesOnScreen--;
                    }
                    break;
                case 'F':
                    if(enemies[i].xPos>playerX){
                        enemies[i].xPos-=trackingEnemySpeed;
                    }else if(enemies[i].xPos<playerX){
                        enemies[i].xPos+=trackingEnemySpeed;
                    }
                    if(enemies[i].yPos>playerY){
                        enemies[i].yPos-=trackingEnemySpeed;
                    }else if(enemies[i].yPos<playerY){
                        enemies[i].yPos+=trackingEnemySpeed;
                    }
            }
            rectangle currentEnemyPos = {enemies[i].xPos, enemies[i].xPos+enemySize, enemies[i].yPos, enemies[i].yPos+enemySize};
            rectangle oldEnemyPos = {enemies[i].oldXPos, enemies[i].oldXPos+enemySize, enemies[i].oldYPos, enemies[i].oldYPos+enemySize};
            
            fill_rectangle(oldEnemyPos, BLACK);
            if(enemies[i].direction!='-') fill_rectangle(currentEnemyPos, WHITE);
        }
    }
}

void detectCollisions(){
    uint8_t e = 0;
    uint8_t b=0;
    for(e; e<maxEnemies; e++){
        if(enemies[e].direction!='-'){
            /*check if enemy hit player*/
            if((playerX<=enemies[e].xPos && playerX+playerSize>=enemies[e].xPos) || (playerX<=enemies[e].xPos+enemySize && playerX+playerSize>=enemies[e].xPos+enemySize)){
                if((playerY<=enemies[e].yPos && playerY+playerSize>=enemies[e].yPos) || (playerY<=enemies[e].yPos+enemySize && playerY+playerSize>=enemies[e].yPos+enemySize)){
                    enemies[e].direction='-';
                    lives--;
                    fill_rectangle((rectangle){enemies[e].xPos, enemies[e].xPos+enemySize, enemies[e].yPos, enemies[e].yPos+enemySize}, BLACK);
                    enemiesOnScreen--;
                }
            }
            /*check if bullet hit enemy*/
            for(b=0; b<maxBullets; b++){
                if(bullets[b].direction!='-'){
                    if((enemies[e].xPos<=bullets[b].xPos && enemies[e].xPos+enemySize>=bullets[b].xPos) || (enemies[e].xPos<=bullets[b].xPos+3 && enemies[e].xPos+enemySize>=bullets[b].xPos+3)){
                        if((enemies[e].yPos<=bullets[b].yPos && enemies[e].yPos+enemySize>=bullets[b].yPos) || (enemies[e].yPos<=bullets[b].yPos+3 && enemies[e].yPos+enemySize>=bullets[b].yPos+3)){
                            enemies[e].direction='-';
                            bullets[b].direction='-';
                            score++;
                            fill_rectangle((rectangle){enemies[e].xPos, enemies[e].xPos+enemySize, enemies[e].yPos, enemies[e].yPos+enemySize}, BLACK);
                            fill_rectangle((rectangle){bullets[b].xPos, bullets[b].xPos+3, bullets[b].yPos, bullets[b].yPos+3}, BLACK);
                            enemiesOnScreen--;
                        }
                    }
                }        
            }
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