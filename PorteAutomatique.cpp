

#include "PorteAutomatique.h"
#include <Arduino.h>

#define MOTOR_INTERFACE_TYPE AccelStepper::FULL4WIRE

PorteAutomatique::PorteAutomatique(int p1, int p2, int p3, int p4, float& distanceRef)
  : _stepper(MOTOR_INTERFACE_TYPE, p1, p3, p2, p4), _distance(distanceRef) {

  _stepper.setMaxSpeed(1000);     
  _stepper.setAcceleration(500);   

  _stepper.setCurrentPosition(_angleEnSteps(_angleFerme));
  _stepper.moveTo(_angleEnSteps(_angleFerme));
  _stepper.enableOutputs();  
  setAngleFerme(10);
  setAngleOuvert(170);
  setPasParTour(2048);
  _actif = true;  
}


void PorteAutomatique::update() {
  _currentTime = millis();
  if (!_actif) return;

  _mettreAJourEtat();

  switch (_etat) {
    case FERMEE: _fermeState(); break;
    case OUVERTE: _ouvertState(); break;
    case EN_OUVERTURE: _ouvertureState(); break;
    case EN_FERMETURE: _fermetureState(); break;
  }

  _stepper.run();
}

// Configuration des angles
void PorteAutomatique::setAngleOuvert(float angle) {
  _angleOuvert = angle;
}

void PorteAutomatique::setAngleFerme(float angle) {
  _angleFerme = angle;
}

// Gestion des distances
void PorteAutomatique::setDistanceOuverture(float distance) {
  _distanceOuverture = distance;
}

float PorteAutomatique::getDistanceOuverture() {
  return _distanceOuverture;
}

void PorteAutomatique::setDistanceFermeture(float distance) {
  _distanceFermeture = distance;
}

float PorteAutomatique::getDistanceFermeture() {
  return _distanceFermeture;
}

// Nombre de pas par tour
void PorteAutomatique::setPasParTour(int steps) {
  _stepsPerRev = steps;
}

float PorteAutomatique::getAngle() const {
  return (_stepper.currentPosition() * 360.0) / _stepsPerRev;
}

const char* PorteAutomatique::getEtatTexte() const {
  switch (_etat) {
    case FERMEE: return "Fermee";
    case OUVERTE: return "Ouverte";
    case EN_OUVERTURE: return "Ouverture...";
    case EN_FERMETURE: return "Fermeture...";
    default: return "Inconnu";
  }
}

void PorteAutomatique::_fermeState() {
  _stepper.disableOutputs();
}

void PorteAutomatique::_ouvertState() {
  _stepper.disableOutputs();
}

void PorteAutomatique::_ouvertureState() {
  if (!_stepper.isRunning()) {
    _etat = OUVERTE;
  }
}

void PorteAutomatique::_fermetureState() {
  if (!_stepper.isRunning()) {
    _etat = FERMEE;
  }
}

void PorteAutomatique::_mettreAJourEtat() {
  if (_etat == FERMEE && _distance < _distanceOuverture && _distance > 0) {
    _etat = EN_OUVERTURE;
    _ouvrir();
  } else if (_etat == OUVERTE && _distance > _distanceFermeture) {
    _etat = EN_FERMETURE;
    _fermer();
  }
}

void PorteAutomatique::_ouvrir() {
  _stepper.enableOutputs();
  _stepper.moveTo(_angleEnSteps(_angleOuvert));
}

void PorteAutomatique::_fermer() {
  _stepper.enableOutputs();
  _stepper.moveTo(_angleEnSteps(_angleFerme));
}

long PorteAutomatique::_angleEnSteps(float angle) const {
  return (long)((_stepsPerRev * angle) / 360.0);
}

void PorteAutomatique::activer() {
  _actif = true;
}

void PorteAutomatique::desactiver() {
  _actif = false;
}

bool PorteAutomatique::estActive() {
  return _actif;
}
