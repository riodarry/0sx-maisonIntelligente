// Fichier : Alarm.h

#pragma once

// États possibles de l’alarme
enum AlarmState {
  OFF,
  WATCHING,
  ON,
  TESTING
};

class Alarm {
public:
  // Constructeur
  // rPin, gPin, bPin : broches pour la DEL RGB
  // buzzerPin : broche du buzzer
  // distancePtr : pointeur vers la variable de distance partagée
  Alarm(int rPin, int gPin, int bPin, int buzzerPin, float* distancePtr);

  // Doit être appelée continuellement dans loop()
  void update();

  // Régle les deux couleurs du gyrophare
  void setColourA(int r, int g, int b);
  void setColourB(int r, int g, int b);

  // Régle la fréquence de variation du gyrophare (en ms)
  void setVariationTiming(unsigned long ms);

  // Régle la distance de déclenchement (en cm)
  void setDistance(float d);
  float getDistance() const;  // Accès à la distance configurée

  // Régle le délai avant extinction après éloignement (en ms)
  void setTimeout(unsigned long ms);
  unsigned long getTimeout() const;  // Accès au délai actuel

  // Allume/éteint manuellement
  void turnOff();
  void turnOn();

  // Déclenche un test de 3 secondes
  void test();

  // Retourne l’état actuel de l’alarme
  AlarmState getState() const;

private:
  // --- Broches matérielles ---
  int _rPin, _gPin, _bPin, _buzzerPin;

  // --- Couleurs du gyrophare ---
  int _colA[3];                // Couleur A
  int _colB[3];                // Couleur B
  bool _currentColor = false;  // false = A, true = B

  // --- Références et seuils ---
  float* _distance;               // Référence vers la distance externe
  float _distanceTrigger = 10.0;  // Seuil de déclenchement

  // --- Temporisations ---
  unsigned long _currentTime = 0;
  unsigned long _variationRate = 500;  // Intervalle entre A/B
  unsigned long _timeoutDelay = 3000;  // Délai après éloignement
  unsigned long _lastUpdate = 0;
  unsigned long _lastDetectedTime = 0;
  unsigned long _testStartTime = 0;

  // --- État actuel ---
  AlarmState _state = OFF;

  // --- Indicateurs de transition ---
  bool _turnOnFlag = false;
  bool _turnOffFlag = false;

  // --- Méthodes internes ---
  void _setRGB(int r, int g, int b);  // Applique une couleur à la DEL
  void _turnOff();                    // Éteint DEL et buzzer

  // --- Méthodes de gestion d’état ---
  void _offState();
  void _watchState();
  void _onState();
  void _testingState();
};
