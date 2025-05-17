
#ifndef PORTE_AUTOMATIQUE_H
#define PORTE_AUTOMATIQUE_H

#include <AccelStepper.h>

class PorteAutomatique {
public:
  PorteAutomatique(int pin1, int pin2, int pin3, int pin4, float& distance);

  void update();  
  bool _modeManuel;

  void setAngleOuvert(float angle);
  void setAngleFerme(float angle);
  void setDistanceOuverture(float distance);
  void setDistanceFermeture(float distance);
  void setPasParTour(int steps);

  float getDistanceOuverture();
  float getDistanceFermeture();
  float getAngle() const;
  const char* getEtatTexte() const;

  // Activation/désactivation
  void activer();
  void desactiver();
  bool estActive();
  void activation();
  void desactivation();

private:
  enum Etat { FERMEE,
              OUVERTE,
              EN_OUVERTURE,
              EN_FERMETURE };
  Etat _etat = FERMEE;

  float& _distance;
  float _distanceOuverture = 20.0;
  float _distanceFermeture = 30.0;
  float _angleOuvert = 170.0;
  float _angleFerme = 10.0;

  int _stepsPerRev = 2048;
  AccelStepper _stepper;

  unsigned long _currentTime = 0;
  bool _actif = false;

  void _mettreAJourEtat();
  void _fermeState();
  void _ouvertState();
  void _ouvertureState();
  void _fermetureState();
  void _ouvrir();  
  void _fermer();  
  long _angleEnSteps(float angle) const;
  
 
};

#endif
