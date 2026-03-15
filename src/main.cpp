#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>

#include "Shaders/Shader.h"
#include "TextureLoader/TextureLoader.h"
#include "Terrain/World/World.h"

// ================== Camera ==================
glm::vec3 cameraPos   = glm::vec3(0.0f, 40.0f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);
float lastX = 800.0f/2.0, lastY = 600.0f/2.0;
float yaw = -90.0f, pitch = -20.0f;
bool firstMouse = true;
float deltaTime = 0.0f, lastFrame = 0.0f;

// ================== Player ==================
glm::vec3 playerPos = cameraPos;
glm::vec3 velocity(0.0f);
bool isGrounded = false;
float gravity = -30.0f, jumpStrength = 12.0f;
glm::vec3 playerSize(0.6f, 1.8f, 0.6f);

// ================== Movement Modes ==================
enum MovementMode {
    NORMAL = 0,
    SPECTATOR = 1
};
MovementMode currentMode = SPECTATOR;

bool togglePressed = false;

// ================== Input ==================
glm::vec3 getMovement(GLFWwindow* window) {
    glm::vec3 move(0.0f);
    
    // Handle movement mode toggle
    if(glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !togglePressed) {
        currentMode = (currentMode == NORMAL) ? SPECTATOR : NORMAL;
        togglePressed = true;
        velocity = glm::vec3(0.0f);
        std::cout << "Movement Mode: " << (currentMode == NORMAL ? "NORMAL" : "SPECTATOR") << std::endl;
    }
    if(glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE) {
        togglePressed = false;
    }

    float speed = (currentMode == SPECTATOR) ? 25.0f * deltaTime : 10.0f * deltaTime;

    // Movement controls - different for each mode
    if(currentMode == SPECTATOR) {
        // Spectator: Full 3D movement using camera direction
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            move += cameraFront * speed;
        }
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            move -= cameraFront * speed;
        }
        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            move -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
        }
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            move += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
        }
        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            move += cameraUp * speed * 2.0f; // Faster vertical movement
        }
        if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            move -= cameraUp * speed * 2.0f;
        }
    } else {
        // Normal mode: Horizontal movement only
        glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
        
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            move += horizontalFront * speed;
        }
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            move -= horizontalFront * speed;
        }
        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            move -= glm::normalize(glm::cross(horizontalFront, cameraUp)) * speed;
        }
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            move += glm::normalize(glm::cross(horizontalFront, cameraUp)) * speed;
        }
        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded) {
            velocity.y = jumpStrength;
            isGrounded = false;
        }
        // Y movement from gravity/jumping
        move.y = velocity.y * deltaTime;
    }

    return move;
}

// ================== Mouse ==================
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if(firstMouse){ lastX=xpos; lastY=ypos; firstMouse=false; }
    float xoffset = xpos-lastX;
    float yoffset = lastY-ypos;
    lastX=xpos; lastY=ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;
    if(pitch>89.0f) pitch=89.0f;
    if(pitch<-89.0f) pitch=-89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw))*cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw))*cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// ================== Resize ==================
void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0,0,width,height);
}

// ================== Collisions ==================
void resolveCollisions(glm::vec3& pos, glm::vec3& move, World& world, bool& isGrounded){
    const float stepSize = 0.01f;

    // Skip collision detection in spectator mode
    if(currentMode == SPECTATOR) {
        pos += move;
        isGrounded = false;
        return;
    }

    isGrounded = false;

    // Helper function to check if position collides with terrain
    auto checkCollision = [&](glm::vec3 testPos) -> bool {
        glm::vec3 playerMin = testPos - glm::vec3(playerSize.x/2, 0, playerSize.z/2);
        glm::vec3 playerMax = testPos + glm::vec3(playerSize.x/2, playerSize.y, playerSize.z/2);
        
        std::vector<glm::vec3> blocks = world.getCollisionBlocks(playerMin, playerMax);
        
        for(const auto& blockPos : blocks) {
            glm::vec3 blockMin(blockPos.x - 0.5f, blockPos.y, blockPos.z - 0.5f);
            glm::vec3 blockMax(blockPos.x + 0.5f, blockPos.y + 1.0f, blockPos.z + 0.5f);

            if(playerMax.x > blockMin.x && playerMin.x < blockMax.x &&
               playerMax.y > blockMin.y && playerMin.y < blockMax.y &&
               playerMax.z > blockMin.z && playerMin.z < blockMax.z) {
                return true;
            }
        }
        return false;
    };

    // Y-axis (vertical movement)
    glm::vec3 newPos = pos;
    newPos.y += move.y;
    
    if(checkCollision(newPos)) {
        if(move.y > 0) {
            newPos.y = pos.y;
            while(!checkCollision(glm::vec3(pos.x, newPos.y + stepSize, pos.z)) && newPos.y < pos.y + abs(move.y)) {
                newPos.y += stepSize;
            }
        } else {
            newPos.y = pos.y;
            while(!checkCollision(glm::vec3(pos.x, newPos.y - stepSize, pos.z)) && newPos.y > pos.y + move.y) {
                newPos.y -= stepSize;
            }
            isGrounded = true;
        }
        velocity.y = 0.0f;
    } else if(move.y <= 0 && !isGrounded) {
        glm::vec3 groundCheck = newPos;
        groundCheck.y -= stepSize;
        if(checkCollision(groundCheck)) {
            isGrounded = true;
            velocity.y = 0.0f;
        }
    }
    
    pos.y = newPos.y;

    // X-axis
    newPos = pos;
    newPos.x += move.x;
    if(!checkCollision(newPos)) pos.x = newPos.x;

    // Z-axis
    newPos = pos;
    newPos.z += move.z;
    if(!checkCollision(newPos)) pos.z = newPos.z;
}

// ================== Main ==================
int main() {
    srand(time(0));

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    GLFWwindow* window = glfwCreateWindow(1980,1080,"Biome Explorer - Infinite World with Caves",monitor,NULL);
    if(!window){ std::cout<<"Failed to create window\n"; glfwTerminate(); return -1;}
    glfwMakeContextCurrent(window);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){ std::cout<<"Failed to init GLAD\n"; return -1;}

    int width,height;
    glfwGetFramebufferSize(window,&width,&height);
    glViewport(0,0,width,height);
    glfwSetFramebufferSizeCallback(window,framebuffer_size_callback);
    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window,mouse_callback);

    glEnable(GL_DEPTH_TEST);

    // ==== Load Biome-appropriate Textures ====
    unsigned int grassTexture = loadTexture("assets/textures/grass.png");     // Plains grass
    unsigned int stoneTexture = loadTexture("assets/textures/stone.png");     // Mountains/caves
    unsigned int dirtTexture  = loadTexture("assets/textures/dirt.png");      // Underground soil
    unsigned int snowTexture  = loadTexture("assets/textures/snow.png");      // Mountain peaks
    
    // Additional textures for enhanced biomes (optional - uncomment if you have them)
    // unsigned int sandTexture = loadTexture("assets/textures/sand.png");    // Desert
    // unsigned int rockTexture = loadTexture("assets/textures/rock.png");    // Cliffs

    // ==== Enhanced Shader ====
    Shader shader("src/Shaders/shader.vert","src/Shaders/shader.frag");
    shader.use();
    // Use the same uniform names as your original shader
    shader.setInt("grassTexture",0);  // Texture unit 0
    shader.setInt("rockTexture",1);   // Texture unit 1 (was rockTexture, now stoneTexture)
    shader.setInt("soilTexture",2);   // Texture unit 2 (was soilTexture, now dirtTexture)
    shader.setInt("snowTexture",3);   // Texture unit 3
    shader.setFloat("gamma",0.7f);
    shader.setFloat("maxHeight", World::MAX_HEIGHT);

    // ==== Enhanced World System ====
    World world;

    std::cout << "=== BIOME EXPLORER - Enhanced World System ===" << std::endl;
    std::cout << "Chunk Size: " << World::CHUNK_SIZE << "x" << World::CHUNK_SIZE << std::endl;
    std::cout << "Render Distance: " << World::RENDER_DISTANCE << " chunks" << std::endl;
    std::cout << "Max Height: " << (int)World::MAX_HEIGHT << " blocks" << std::endl;
    std::cout << "\n=== BIOMES ===" << std::endl;
    std::cout << "🌱 Plains - Rolling grasslands with gentle hills" << std::endl;
    std::cout << "🏔️  Mountains - Towering peaks with snow caps" << std::endl;
    std::cout << "🗻 Hills - Varied terrain with mixed grass/stone" << std::endl;
    std::cout << "🏜️  Desert - Sandy dunes and flatlands" << std::endl;
    std::cout << "🕳️  Caves - Underground cavern systems" << std::endl;
    std::cout << "\n=== CONTROLS ===" << std::endl;
    std::cout << "TAB - Toggle Normal/Spectator mode" << std::endl;
    std::cout << "WASD - Move around" << std::endl;
    std::cout << "Normal: SPACE - Jump" << std::endl;
    std::cout << "Spectator: SPACE - Up, SHIFT - Down" << std::endl;
    std::cout << "\nExplore the world to find different biomes and cave systems!" << std::endl;

    GLint isOutlineLoc = glGetUniformLocation(shader.ID,"isOutline");

    // ==== Main Loop ====
    while(!glfwWindowShouldClose(window)){
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Enhanced status display with biome detection
        static float lastPrint = 0;
        if(currentFrame - lastPrint > 4.0f) {
            int chunkX = static_cast<int>(floor(playerPos.x / World::CHUNK_SIZE));
            int chunkZ = static_cast<int>(floor(playerPos.z / World::CHUNK_SIZE));
            int height = world.getHeightAtWorld((int)playerPos.x, (int)playerPos.z);
            
            // Estimate current biome based on terrain height
            std::string currentBiome;
            if (height < 12) {
                if (height < 8) currentBiome = "🏜️ Desert";
                else currentBiome = "🌱 Plains";
            } else if (height < 30) {
                currentBiome = "🗻 Hills";
            } else {
                currentBiome = "🏔️ Mountains";
            }
            
            // Check if player is in a cave
            bool inCave = world.isCaveAtWorld((int)playerPos.x, (int)playerPos.y, (int)playerPos.z);
            if (inCave) {
                currentBiome += " 🕳️ (In Cave)";
            }
            
            std::cout << "Mode: " << (currentMode == NORMAL ? "NORMAL" : "SPECTATOR") 
                      << " | Pos: " << (int)playerPos.x << ", " << (int)playerPos.y << ", " << (int)playerPos.z
                      << " | Chunk: " << chunkX << "," << chunkZ 
                      << " | Surface Height: " << height 
                      << " | Biome: " << currentBiome << std::endl;
            lastPrint = currentFrame;
        }

        // Apply gravity only in normal mode
        if(currentMode == NORMAL) {
            velocity.y += gravity * deltaTime;
        }

        // Update world chunks around player
        world.updateChunks(playerPos);

        // Compute movement and handle collisions
        glm::vec3 move = getMovement(window);
        resolveCollisions(playerPos, move, world, isGrounded);

        // Set camera at eye level
        cameraPos = playerPos + glm::vec3(0.0f, 1.6f, 0.0f);

        // Dynamic sky color based on height and time
        float skyBlend = glm::clamp(playerPos.y / 60.0f, 0.3f, 1.0f);
        glm::vec3 skyColor = glm::mix(glm::vec3(0.6f, 0.8f, 1.0f), glm::vec3(0.8f, 0.9f, 1.0f), skyBlend);
        
        // Darken sky if underground/in caves
        if (playerPos.y < world.getHeightAtWorld((int)playerPos.x, (int)playerPos.z)) {
            skyColor = glm::mix(skyColor, glm::vec3(0.1f, 0.1f, 0.2f), 0.7f);
        }
        
        glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Setup rendering with correct texture binding order
        shader.use();
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, grassTexture);  // grassTexture uniform
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, stoneTexture);  // rockTexture uniform  
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, dirtTexture);   // soilTexture uniform
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, snowTexture);   // snowTexture uniform

        // Calculate matrices
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glfwGetFramebufferSize(window, &width, &height);
        glm::mat4 projection = glm::perspective(glm::radians(75.0f), width/(float)height, 0.1f, 1000.0f);

        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        
        // Enhanced fog system - varies with height and environment
        float fogDistance = 200.0f + (playerPos.y * 3.0f); // Further view at height
        
        // Reduce fog in caves/underground
        if (playerPos.y < world.getHeightAtWorld((int)playerPos.x, (int)playerPos.z)) {
            fogDistance *= 0.3f; // Much closer fog underground
        }
        
        shader.setVec3("fogColor", skyColor);
        shader.setFloat("fogStart", fogDistance);
        shader.setFloat("fogEnd", fogDistance + 400.0f);

        // Render the world
        glUniform1i(isOutlineLoc, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        world.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}