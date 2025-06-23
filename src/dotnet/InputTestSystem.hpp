#pragma once

#include "Engine.hpp"
#include "InputWrapper.hpp"
#include "Components.hpp"
#include <iostream>

/**
 * Example system that demonstrates how to use the Input system in scripts
 */
class InputTestSystem {
public:
    static void CreatePlayerControllerSystem(App& app) {
        app.add_update_system([&app](float deltaTime) {
            auto view = app.registry.view<Transform, Mesh>();
            
            for (auto entity : view) {
                auto& transform = view.get<Transform>(entity);
                auto& mesh = view.get<Mesh>(entity);
                
                // Only control monkey entity (modelId == 0) for this example
                if (mesh.modelId == 0) {
                    float moveSpeed = 2.0f * deltaTime;  // 2 units per second
                    
                    // Use Input system to check for WASD keys
                    if (Input::GetKey_W()) {
                        transform.position.z -= moveSpeed;  // Forward
                        std::cout << "Moving forward! Position: (" 
                                 << transform.position.x << ", " 
                                 << transform.position.y << ", " 
                                 << transform.position.z << ")\n";
                    }
                    if (Input::GetKey_S()) {
                        transform.position.z += moveSpeed;  // Backward
                        std::cout << "Moving backward! Position: (" 
                                 << transform.position.x << ", " 
                                 << transform.position.y << ", " 
                                 << transform.position.z << ")\n";
                    }
                    if (Input::GetKey_A()) {
                        transform.position.x -= moveSpeed;  // Left
                        std::cout << "Moving left! Position: (" 
                                 << transform.position.x << ", " 
                                 << transform.position.y << ", " 
                                 << transform.position.z << ")\n";
                    }
                    if (Input::GetKey_D()) {
                        transform.position.x += moveSpeed;  // Right
                        std::cout << "Moving right! Position: (" 
                                 << transform.position.x << ", " 
                                 << transform.position.y << ", " 
                                 << transform.position.z << ")\n";
                    }
                    if (Input::GetKey_Q()) {
                        transform.position.y += moveSpeed;  // Up
                        std::cout << "Moving up! Position: (" 
                                 << transform.position.x << ", " 
                                 << transform.position.y << ", " 
                                 << transform.position.z << ")\n";
                    }
                    if (Input::GetKey(KeyCode::Z)) {  // Using direct keycode
                        transform.position.y -= moveSpeed;  // Down
                        std::cout << "Moving down! Position: (" 
                                 << transform.position.x << ", " 
                                 << transform.position.y << ", " 
                                 << transform.position.z << ")\n";
                    }
                    
                    // Example of GetKeyDown (only triggers once per press)
                    if (Input::GetKeyDown_Space()) {
                        std::cout << "Space was just pressed! Current position: (" 
                                 << transform.position.x << ", " 
                                 << transform.position.y << ", " 
                                 << transform.position.z << ")\n";
                    }
                    
                    // Mouse input example
                    if (Input::GetMouseButtonDown(MouseButton::Left)) {
                        auto mousePos = Input::GetMousePosition();
                        std::cout << "Left mouse button clicked at: (" 
                                 << mousePos.x << ", " << mousePos.y << ")\n";
                    }
                    
                    break;  // Only control the first monkey we find
                }
            }
        });
    }
}; 