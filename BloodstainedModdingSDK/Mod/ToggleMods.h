#pragma once
#include "IToggleMod.h"

class UnlimitedStrengthMod : public IToggleMod {
public:
    void OnActivated() override;
    void OnDeactivated() override;
private: 
  float originalValue;
};

class UnlimitedLuckMod : public IToggleMod {
public:
    void OnActivated() override;
    void OnDeactivated() override;
private:
  float originalValue;
};

class UnlimitedIntMod : public IToggleMod {
public:
    void OnActivated() override;
    void OnDeactivated() override;
private:
  float originalValue;
};


class UnlimitedConMod : public IToggleMod {
public:
    void OnActivated() override;
    void OnDeactivated() override;
private:
  float originalValue;
};


class UnlimitedMindMod : public IToggleMod {
public:
    void OnActivated() override;
    void OnDeactivated() override;
private:
  float originalValue;
};


class UnlimitedSpeedMod : public IToggleMod {
public:
    void OnActivated() override;
    void OnDeactivated() override;
private:
  float originalValue;
};
