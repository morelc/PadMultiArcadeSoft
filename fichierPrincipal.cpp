/** 
 * Copyright (C) 2015 MOREL Charles <charles.morel@iut-valence.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 **/


/**-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * ######################################################################### PARAMETRES DE L'APPLICATION ######################################################################### 
 -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------**/

#define MIN_TIMER_FOR_AUTOFIRE 50 // en ms
#define MAX_PROFILS_IN_PAD 20 // en fonction de la mémoire disponible

#define NUM_FIRST_PIN_LCD 2
#define NUM_PIN_LCD_BACKLIGHT A13
#define NUM_FIRST_PIN_IHM 38
#define NUM_PIN_NEXT_PROFIL A14
#define NUM_PIN_PREVIOUS_PROFIL A15
#define NUM_FIRST_PIN_COMPONENTS_A 22
#define NUM_FIRST_PIN_COMPONENTS_B 30
#define NUM_FIRST_PIN_SDCARD 50

#define ARRAY_CORRESP_ID_MACHINE {"NES","SNES","ATARI 2600"}
#define PROFILS_FILE_NAME "config.bin"

/**------------------------------ Définitions pour netbeans ------------------------------**/
#define NULL NULL
#define Serial Serial
#define strncpy strncpy
#define strcmp strcmp
#define strchr strchr

/**-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * ########################################################################### DEFINITION DES CLASSES ########################################################################### 
 -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------**/

/**------------------------------ CLASSE BtnAssoc ------------------------------*/
class BtnAssoc {
public:
  bool isAutofire;
  int timer;
  int signalToSend;
  bool hasNext;
  BtnAssoc* nextAssoc;

  BtnAssoc() {
    isAutofire = false;
    timer = 0;
    signalToSend = 0;
    hasNext = false;
    nextAssoc = NULL;
  }
  BtnAssoc* clone()
  {
    BtnAssoc* newBtnAssoc = new BtnAssoc();
    newBtnAssoc->isAutofire = isAutofire;
    newBtnAssoc->timer = timer;
    newBtnAssoc->signalToSend = signalToSend;
    newBtnAssoc->hasNext = hasNext;
    newBtnAssoc->nextAssoc = nextAssoc;
    return newBtnAssoc;
  }

  void serialPrint() {
    Serial.print("Autofire=");
    Serial.print(isAutofire);
    Serial.print(" - Timer=");
    Serial.print(timer);
    Serial.print(" - Signal=");
    Serial.print(signalToSend);
    if (hasNext) {
      Serial.print(" => ");
      nextAssoc->serialPrint();
    } 
    else
      Serial.print(" ||");
  }
};

/*------------------------------ CLASSE Profil  et dérivés ------------------------------*/

class Profil {
public:
  char name[17];
  int idMachine;
  BtnAssoc* assocs[12];

  Profil() {
    strncpy(name, "UNNAMED_PROFIL", sizeof (name));
    idMachine = -1;
    for (int i = 0; i < 12; i++)
      assocs[i] = new BtnAssoc();
  }

  void sendSignal();
  void initPins() {  
  };

  Profil* clone()
  {
    Profil* newProfil = new Profil();
    strncpy(newProfil->name, name, sizeof (newProfil->name));
    newProfil->idMachine = idMachine;
    for (int i = 0; i < 12; i++)
      newProfil->assocs[i] = assocs[i]->clone();
    return newProfil;
  }

  void displayOnLCDScreen() // à définir au niveau lcd
  {
    char* machineNanes[] = ARRAY_CORRESP_ID_MACHINE;
    Serial.println(name);
    Serial.println(machineNanes[idMachine]);
  }

  void serialPrint() {
    Serial.print("Name=");
    Serial.print(name);
    Serial.print(" - idMachine=");
    Serial.print(idMachine);
    for (int i = 0; i < 12; i++) {
      Serial.println();
      assocs[i]->serialPrint();
    }
  }

private:
  void sendInstantSignal(); // à définir
  void sendAutofireSignal(); // à définir
  void sendMacro(); // à définir
};

class SNESProfil : Profil
{
  void sendSignal();
  void initPins()
  {
    for(int i = 0; i < 8; i++)
    {
      pinMode(NUM_FIRST_PIN_COMPONENTS_A+i, OUTPUT);
      pinMode(NUM_FIRST_PIN_COMPONENTS_B+i, OUTPUT);
    }  
  }
};
/*------------------------------ CLASSE BtnAssocBuilder ------------------------------*/

class BtnAssocBuilder {
private:
  BtnAssoc *firstAssoc;
  BtnAssoc *currentAssoc;

public:

  BtnAssocBuilder() {
    resetBtnAssoc();
  }

  BtnAssoc* getBtnAssoc() {
    BtnAssoc* btnAssocToReturn = firstAssoc;
    resetBtnAssoc();
    return btnAssocToReturn;
  }

  void resetBtnAssoc() {
    firstAssoc = new BtnAssoc();
    currentAssoc = firstAssoc;
  }

  void activateAutofire() {
    BtnAssoc* newBtnAssoc = new BtnAssoc();
    newBtnAssoc->isAutofire = true;
    newBtnAssoc->hasNext = false;
    newBtnAssoc->nextAssoc = NULL;
    newBtnAssoc->signalToSend = firstAssoc->signalToSend;
    newBtnAssoc->timer = firstAssoc->timer;
    if (newBtnAssoc->timer < MIN_TIMER_FOR_AUTOFIRE)
      newBtnAssoc->timer = MIN_TIMER_FOR_AUTOFIRE;
    firstAssoc = newBtnAssoc;
    currentAssoc = newBtnAssoc;
  }

  void setSignalToSend(int theNewSignal) {
    currentAssoc->signalToSend = theNewSignal;
  }

  void setTimer(int theNewTimer) {
    if ((currentAssoc->isAutofire && theNewTimer > MIN_TIMER_FOR_AUTOFIRE) || (!currentAssoc->isAutofire && theNewTimer > 0))
      currentAssoc->timer = theNewTimer;
  }

  void createNext() {
    currentAssoc->isAutofire = false;
    currentAssoc->hasNext = true;
    currentAssoc->nextAssoc = new BtnAssoc();
    currentAssoc = currentAssoc->nextAssoc;
  }

};

/*------------------------------ CLASSE ProfilBuilder ------------------------------*/

class ProfilBuilder {
private:
  int idMachineProfil;
  char profilName[17];
  BtnAssoc* assocsProfil[12];

public:

  ProfilBuilder() {
    resetProfil();
  }

  Profil* getProfil() {
    Profil* profilToReturn = new Profil();
    strncpy(profilToReturn->name, profilName, sizeof(profilName));
    profilToReturn->idMachine = idMachineProfil;
    for (int i = 0; i < 12; i++)
      profilToReturn->assocs[i] = assocsProfil[i];
    resetProfil();
    return profilToReturn;
  }

  void resetProfil() {
    strncpy(profilName, "UNNAMED_PROFIL", sizeof (profilName));
    idMachineProfil = -1;
    for (int i = 0; i < 12; i++)
      assocsProfil[i] = new BtnAssoc();
  }

  void setProfilName(char theNewProfilName[17]) {
    strncpy(profilName, theNewProfilName, sizeof (profilName));
  }

  void setMachine(int theNewIdMachine) {
    idMachineProfil = theNewIdMachine;
  }

  void setAssoc(int idBtn, BtnAssoc* theNewAssoc) {
    assocsProfil[idBtn] = theNewAssoc->clone();
  }

};

/*------------------------------ CLASSE Data ------------------------------*/
class Data
{
private:
  int currentProfile;
  int nbOfProfils;
  Profil* profils[MAX_PROFILS_IN_PAD];

public: 
  int lastBtnPressed;
  int autofireActivated;

  Data()
  {
    currentProfile = 0;
    lastBtnPressed = 0;
    nbOfProfils = 1;
    for(int i = 0; i < MAX_PROFILS_IN_PAD; i++)
      profils[i] = new Profil();
  }
  void nextProfil()
  {
    currentProfile = (currentProfile + 1)%nbOfProfils;
    startProfil();
  }
  void previousProfil()
  {
    currentProfile = (currentProfile - 1 + nbOfProfils)%nbOfProfils;
    startProfil();
  }
  void startProfil()
  {
    Serial.print("Profil actuel: ");
    Serial.println(currentProfile);
    profils[currentProfile]->displayOnLCDScreen();
    profils[currentProfile]->initPins();
  }
  Profil* getCurrentProfil()
  {
    return profils[currentProfile];
  }
  void loadProfilsFromSDCard()// à définir
  {
    // note: nom du fichier = PROFILS_FILE_NAME
    nbOfProfils = 2;
    BtnAssocBuilder *bab;
    ProfilBuilder *pb;
    bab = new BtnAssocBuilder();
    pb = new ProfilBuilder();
    pb->setProfilName("cool");
    pb->setMachine(1);
    bab->setSignalToSend(10);
    bab->createNext();
    bab->setSignalToSend(5);
    bab->setTimer(25);
    pb->setAssoc(1, bab->getBtnAssoc()); 
    bab->setTimer(80);
    bab->activateAutofire();
    bab->setSignalToSend(7);
    pb->setAssoc(5, bab->getBtnAssoc());
    profils[0] = pb->getProfil();
    pb->setProfilName("cool bis");
    pb->setMachine(2);
    bab->setSignalToSend(5);
    bab->createNext();
    bab->setSignalToSend(2);
    pb->setAssoc(9, bab->getBtnAssoc());
    bab->activateAutofire();
    bab->setTimer(2);
    bab->setSignalToSend(21);
    pb->setAssoc(6, bab->getBtnAssoc());
    profils[1] = pb->getProfil();
  }

  void autoselectProfile(); // à définir, doit lire l'EPROM  afin de trouver l'id du dernier profil sélectionné, copie le num dans currentProfile et appel startProfil(). Si num > au nombre de profils alors on 
};

/*------------------------------ CLASSE IHM ------------------------------*/

class IHM
{
public:
  int quantifyPressedBtn()
  {
    int pinsPressedValue = 0;
    for(int i = NUM_FIRST_PIN_IHM; i < NUM_FIRST_PIN_IHM + 12; i++)
      pinsPressedValue = serialPinRead(i,pinsPressedValue);
    pinsPressedValue = serialPinRead(NUM_PIN_NEXT_PROFIL,pinsPressedValue);
    pinsPressedValue = serialPinRead(NUM_PIN_PREVIOUS_PROFIL,pinsPressedValue);
    return pinsPressedValue;
  }
  void changeProfil(Data currentProfilDatas); // à définir
  void initLCDPins(); // à définir (initialise les pins du LCD + rétroéclairage puis met le LCD à OFF avec turnOffLCD() )
  void initIHMPins()
  {
    for(int i = NUM_FIRST_PIN_IHM; i < NUM_FIRST_PIN_IHM + 12; i++)
      pinMode(i, INPUT);
    pinMode(NUM_PIN_NEXT_PROFIL, INPUT);
    pinMode(NUM_PIN_NEXT_PROFIL, INPUT);  
  }
  void showMessageOnLCD(char messageLine1[17], char messageLine2[17]); // à définir
  void turnOnLCD()
  {
    digitalWrite(NUM_PIN_LCD_BACKLIGHT, HIGH);
  }
  void turnOffLCD()
  {
    digitalWrite(NUM_PIN_LCD_BACKLIGHT, LOW);
  }
private:
  int serialPinRead(int numPinToRead, int currentPinsValues)
  {
    return ((currentPinsValues << 1) + digitalRead(numPinToRead));
  }
};
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 ############################################################################ PROGAMME PRINCIPAL ############################################################################ 
 -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

/*------------------------------ Définition des variables globales ------------------------------*/
Data *padDatas;
IHM *padIHM;

/*------------------------------ Définition de setup ------------------------------*/
void setup() {
  Serial.begin(9600);
  padDatas = new Data();
  padIHM = new IHM();
  padDatas->loadProfilsFromSDCard();
  Serial.println("Veuillez entrer votre commande...");
  /**
   * padDatas = new Data();
   * padIHM = new IHM();
   * padIHM->turnOnLCD();
   * padIHM->showMessageOnLCD("Salut!","   Chargement...");
   * padDatas->loadProfilsFromSDCard();
   * padDatas->autoselectProfile();
   */

}

/*------------------------------ Définition de loop ------------------------------*/
void loop() {
  terminal();


}

/*------------------------------ Définition des fonctions permettant les tests ------------------------------*/
void terminal()
{
  char cmd[2];
  Serial.readBytes(cmd, 1);
  nettoyerCommande((char*) &cmd);
  if (strcmp(cmd, "+") == 0)
    padDatas->nextProfil();
  if (strcmp(cmd, "-") == 0)
    padDatas->previousProfil();
  if(strcmp(cmd, "g") == 0)
    padDatas->getCurrentProfil()->serialPrint();
  if(strcmp(cmd, "p") == 0)
    padDatas->getCurrentProfil()->displayOnLCDScreen();
}

void nettoyerCommande(char* commandeANettoyer) {
  char *pos;
  if ((pos = strchr(commandeANettoyer, '\n')) != NULL)
    *pos = '\0';
}












