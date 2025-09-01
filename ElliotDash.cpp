#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include "GameOver.hpp"
#include "StartScreen.hpp"
#include "GameState.hpp"
#include <random>
#include <vector>
#include <optional>
#include <iostream>

int main()
{

    // Window Setup
    sf::RenderWindow window(sf::VideoMode({800,600}), "Elliot's Game");
    
    bool isPortal = false;
    bool portalActive = false;

    sf::Texture portalTexture;
    if(!portalTexture.loadFromFile("newPortalSheet.png")){

        std::cerr << "Failed to load newPortalSheet.png";

    }

    const int framesX = 4;
    const int framesY = 4;
    const int portalTotal = framesX * framesY;

    int frameX = 0;
    int frameY = 0;
    int count = 0;

    const auto pTS = portalTexture.getSize();
    const int pX = static_cast<int>(pTS.x) / framesX;
    const int pY = static_cast<int>(pTS.y) / framesY;

    sf::Sprite portalSprite(portalTexture);

    portalSprite.setTextureRect({{0, 0}, {pX, pY}});
    portalSprite.setOrigin({static_cast<float>(pX), static_cast<float>(pY)});

    int   portalFrame  = 0; 
    float portalFPS    = 8.f;         
    sf::Vector2f portalPos(800.f, 600.f);
    sf::Clock portalAnimClock;

    auto setPortalFrame = [&](int f){
        const int cx = f % framesX;
        portalSprite.setTextureRect({{cx * pX, 0 * pY}, {pX, pY}});
    };
    setPortalFrame(0);

    sf::Texture bgTexture;
    if(!bgTexture.loadFromFile("Night.jpg")){

        std::cerr << "Failed to load Background.jpg";

    }
    sf::Sprite bg(bgTexture);
    const float targetW = 800.f;
    const float targetH = 600.f;
    auto ts = bgTexture.getSize();    
    float sx = targetW / static_cast<float>(ts.x); 
    float sy = targetH / static_cast<float>(ts.y);
    bg.setScale({sx, sy});

    GameOver gameOver;
    StartScreen startScreen;
    GameState gameState = GameState::Start;

    float highScore;
    float currentScore;

    sf::Texture texture;
    if (!texture.loadFromFile("Shinobi/Run.png")) {
        std::cerr << "Failed to load Run.png\n";
    }

    int runState = 8;
    int frame = 0;

    auto sTS = texture.getSize();
    int sX = static_cast<int>(sTS.x) / runState;
    const int sY = 128;

    sf:: Sprite samurai(texture);
    samurai.setTextureRect({{0, 0}, {sX, sY}});
    samurai.setScale({0.65, 0.65});
    samurai.setOrigin({sX * 0.5f, static_cast<float>(sY)});
    samurai.setPosition({150.f, 600.f}); 

    // Sprite constants and variable elements

    const float ground = 564.f;
    const float blockGround = 524.f;
    const float blockCeiling = 100.f;

    float velocityY = 0.f;
    const float gravity = 2000.f;
    const float jumpImpulse = -1000.f;
    const float blockJumpImpulse = 900.f;
    const float gDown = 600.f;
    const float vyClamp = 900.f;
    const float maxFall = 2000.f;

    const float maxHoldTime = 0.18f;
    const float holdGravityScaler = 0.35f;
    const float releaseCut = 0.45f;

    const float coyoteTime = 0.08f;
    const float jumpBuff = 0.10f;

    float coyoteTimer = 0.f;
    float buffTimer = 0.f;
    bool jumpHeld = false;
    float holdTime = 0.f;

    bool grounded = true;
    bool blockGroundReached = true;
    bool blockCeilingReached = true;

    // Random dimensions for incoming block initiation

    std::random_device rndm;
    std::mt19937 gen(rndm());
    std::uniform_real_distribution<float> widthDist(50.f, 150.f);
    std::uniform_real_distribution<float> heightDist(30.f, 150.f);
    std::uniform_real_distribution<float> oneTwo(0.f, 3.f);

    // Create blocks container

    std::vector<sf::RectangleShape> blocks;

    struct GravityBlock {

        sf::RectangleShape shape;
        float vy = 0.f;

    };
    std::vector<GravityBlock> gravityBlocks;

    // Create block

    bool gravityBlock = false;
    float width = widthDist(gen);
    float height = heightDist(gen);
    float isFloating = oneTwo(gen);
    sf::RectangleShape block;
    block.setSize({width, height});
    sf::Vector2f size = block.getSize();
    block.setOrigin({size.x, size.y});
    block.setPosition({800.f, 464.f});
    blocks.push_back(block);

    float blockSpeed = 250.f;
    float blockSpawnTime = 2.f;

    // Clock initiation 

    sf::Clock blockMoveClock; 
    sf::Clock newBlockClock;
    sf::Clock gameClock;
    sf::Clock scoreClock;
    sf::Clock gravityClock;
    sf::Clock blockGravityClock;
    sf::Clock runStateClock;
    sf::Clock portalClock;

    auto resetGame = [&] {

        window.clear();
        blockSpawnTime = 2.f;
        blockSpeed = 250.f;
        blocks.clear();
        gravityBlocks.clear();
        scoreClock.restart(); 
        isPortal = false; 
        portalActive = false;
        frameX = 0;
        frameY = 0;
        count = 0;

        portalSprite.setPosition({800.f, 600.f});
        samurai.setPosition({150.f, ground});
        velocityY = 0.f;
        grounded = true;
        coyoteTimer = 0.f;
        buffTimer = 0.f;
        jumpHeld = false;
        holdTime = 0.f;
        gravityClock.restart();
    
    };

    // Game loop
    while (window.isOpen()) {
    
        if (gameState == GameState::Start) {
            
            bool begin = startScreen.show(window);   
            if (!begin) { window.close(); break; }
            resetGame();                 
            gameState = GameState::Playing;
            continue;                    
        }

        // Poll event loop
        while (const std::optional<sf::Event> event = window.pollEvent()) {

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }   else if (event->is<sf::Event::KeyPressed>()){
       
                const auto* key = event->getIf<sf::Event::KeyPressed>();

                if(key && key->code == sf::Keyboard::Key::Space && samurai.getPosition().y > 450) {

                    buffTimer = jumpBuff;

                }else if(key && key->code == sf::Keyboard::Key::Right && !isPortal){

                    for(auto& b : blocks){

                        b.setPosition({b.getPosition().x - 250.f, b.getPosition().y});

                    }
                    for(auto& b : gravityBlocks){

                        b.shape.setPosition({b.shape.getPosition().x - 250.f, b.shape.getPosition().y});

                    }

                }
             } else if(event->is<sf::Event::KeyReleased>()){
                const auto* key = event->getIf<sf::Event::KeyReleased>();
                if(key && key->code == sf::Keyboard::Key::Space){

                    jumpHeld = false;
                    if(velocityY < 0.f){

                        velocityY *= releaseCut;

                    }

                } else if(key && key->code == sf::Keyboard::Key::Right){

                    for(auto& b : blocks){

                        b.setPosition({b.getPosition().x , b.getPosition().y});

                    }
                    for(auto& b : gravityBlocks){

                        b.shape.setPosition({b.shape.getPosition().x, b.shape.getPosition().y});

                    }

                }

             }
        }

    sf::Time scoreClockTime = scoreClock.getElapsedTime();
    currentScore = scoreClockTime.asSeconds();
    sf::Font scoreFont;
    if(!scoreFont.openFromFile("AmericanCaptain.otf")) return -1;
    sf::Text score(scoreFont);

    if(currentScore < 10.f){

        score.setString("0:0" + std::to_string(currentScore));
        score.setFillColor(sf::Color::Red);
        score.setCharacterSize(32);
        score.setPosition({120.f, 120.f});

    }else if(currentScore >= 10.f && currentScore < 60.f){

        score.setString("0:" + std::to_string(currentScore));
        score.setFillColor(sf::Color::Red);
        score.setCharacterSize(32);
        score.setPosition({120.f, 120.f});

    }

       float currentPosY = samurai.getPosition().y;
       float currentPosX = samurai.getPosition().x;

       float dt = gravityClock.restart().asSeconds();

        // Timers tick down
        if (coyoteTimer > 0.f) coyoteTimer -= dt;
        if (buffTimer > 0.f) buffTimer -= dt;

        // Ground check (based on last frame’s position)
        grounded = (samurai.getPosition().y >= ground - 0.5f);
        if (grounded) {

            coyoteTimer = coyoteTime;
            holdTime = 0.f;

        }

        // Consume a buffered jump if allowed (on ground or within coyote window)
        if (buffTimer > 0.f && coyoteTimer > 0.f) {

            velocityY = jumpImpulse;       // GO UP!
            grounded = false;
            buffTimer = 0.f;      // consume
            coyoteTimer = 0.f;      // leaving ground
            holdTime = 0.f;

        }

        // Apply gravity
        if (velocityY < 0.f && jumpHeld && holdTime < maxHoldTime) {
            velocityY += (gravity * holdGravityScaler) * dt;   // gentler gravity during hold
            holdTime += dt;
        } else {
            velocityY += gravity * dt;                         // normal gravity
        }

        // clamp fall speed
        if (velocityY > maxFall) velocityY = maxFall;

        // Move sprite by velocity
        samurai.move({0.f, velocityY * dt});

        // Ground clamp
        if (samurai.getPosition().y > ground) {
            samurai.setPosition({samurai.getPosition().x, ground});
            velocityY = 0.f;
            grounded = true;
        }if(frame >= 7 && grounded){

            frame = 0;

        }else if(grounded){

            frame += 1;

        }else{

            frame = 2;

        }

        sf::Time runStateTime = runStateClock.getElapsedTime();
        float rsDT = runStateTime.asSeconds();

        if(rsDT >= 0.1){
            samurai.setTextureRect({{frame * sX, 0}, {sX, sY}});
            runStateClock.restart();
        }

        sf::Time pT = portalClock.getElapsedTime();
        float portalTimer = pT.asSeconds();

        if(currentScore >= 10.f && !isPortal && !portalActive){

            isPortal = true;
            portalActive = true;
            portalFrame = 0;
            portalPos = {800.f, 600.f};
            setPortalFrame(portalFrame);
            portalAnimClock.restart();

        }

       sf::Time time = newBlockClock.getElapsedTime();
       float timeInSec = time.asSeconds();

       if(timeInSec >= blockSpawnTime && !isPortal){

        gravityBlock = false;
        float width = widthDist(gen);
        float height = heightDist(gen);
        float isFloating = oneTwo(gen);
        sf::RectangleShape block;
        block.setSize({width, height});
        sf::Vector2f size = block.getSize();
        block.setOrigin({size.x, size.y});
        if(isFloating > 1.6 && isFloating < 2.5f){

            block.setPosition({800.f, 464.f});

        }else if (isFloating <= 1.6){

            block.setPosition({800.f, 564.f});

        }else{

            gravityBlock = true;
            block.setPosition({800.f, 524.f});

        }
        if(gravityBlock){

            GravityBlock b;
            b.shape = block;
            b.vy = -blockJumpImpulse;
            gravityBlocks.push_back(b);

        }else{

            blocks.push_back(block);

        }
        newBlockClock.restart();

       }

       // Keep track of total time playing to make the block speed imcrease

       sf::Time gameTime = gameClock.getElapsedTime();
       float gcSec = gameTime.asSeconds();

       float delta = blockMoveClock.restart().asSeconds();

       if(isPortal){

        portalPos.x -= 200.f *delta;
        portalSprite.setPosition(portalPos);

        if (portalAnimClock.getElapsedTime().asSeconds() >= 1.f / portalFPS) {

            portalAnimClock.restart();

            if (portalFrame < portalTotal - 1) {

                ++portalFrame;
                setPortalFrame(portalFrame);

            }
            
        }
    
        if (portalPos.x + pX * 0.5f < 0.f) {
            isPortal = false;
        }

        blocks.clear();
        gravityBlocks.clear();

       }

       if(gcSec >= 2.f){

        if(blockSpeed <= 500){

            blockSpeed+= 50.f;

        }
            
            gameClock.restart();
            if(blockSpawnTime >= 0.8f){

                blockSpawnTime -= 0.2f;

            }

       }

            for(auto& block : blocks){

                    block.move({-blockSpeed * delta, 0.f});

            }

       float blockDT = blockGravityClock.restart().asSeconds();

            for(auto& block :gravityBlocks){

                block.shape.move({-blockSpeed * delta, 0.f});

                block.vy += gravity *blockDT;

                if(block.vy > vyClamp){

                    block.vy = vyClamp;

                }
                if(block.vy < -vyClamp){

                    block.vy = -vyClamp;

                }

                 block.shape.move({0.f, block.vy * blockDT});

                blockGroundReached = (block.shape.getPosition().y >= blockGround - 0.5f);
                blockCeilingReached = (block.shape.getPosition().y <= blockCeiling + 0.5f);

                if (blockGroundReached) {

                    block.vy = -blockJumpImpulse;

                }else if(blockCeilingReached){

                    block.vy = gDown;

                }

            }

       // Check if user has been hit
       bool stateChanged = false;

    for (auto& block : blocks) {
        if (samurai.getGlobalBounds().findIntersection(block.getGlobalBounds())) {
            GameState result = gameOver.show(window);

            if (result == GameState::Playing) {
                resetGame();
                gameState = GameState::Playing;
            } else { 
                resetGame();
                gameState = GameState::Start;
            }

            stateChanged = true;
            break; // leave the blocks loop
    }
}

for (auto& block : gravityBlocks) {
    if (samurai.getGlobalBounds().findIntersection(block.shape.getGlobalBounds())) {
        GameState result = gameOver.show(window);

        if (result == GameState::Playing) {
            resetGame(); 
            gameState = GameState::Playing;
        } else {
            resetGame();
            gameState = GameState::Start;
        }

        stateChanged = true;
        break; // leave the blocks loop
}
}

    if (stateChanged) continue;


    window.clear();

    window.draw(bg);

    window.draw(score);

    window.draw(samurai);

    if(isPortal){
        window.draw(portalSprite);
    }

    for(auto& block : blocks) window.draw(block);

    for(auto& block : gravityBlocks) window.draw(block.shape);

    window.display();

    }

}