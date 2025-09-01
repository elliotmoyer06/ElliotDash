#pragma once
#include <SFML/Graphics.hpp>
#include "GameState.hpp"
#include <optional>
#include <string>

class GameOver{

public:
    GameState show(sf::RenderWindow& window);

private:
    bool loadFont();
    sf::Font font_;
};