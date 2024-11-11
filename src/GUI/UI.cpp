#include <SFML/Graphics.hpp>

int RunUI() {
    // Create a window with dimensions 1800x1800 pixels
    sf::RenderWindow window(sf::VideoMode(1800, 1800), "Drone Visualization");

    // Main loop that continues until the window is closed
    while (window.isOpen()) {
        // Process events
        sf::Event event;
        while (window.pollEvent(event)) {
            // Close the window when the close button is clicked
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Clear the window with a black color
        window.clear(sf::Color::Black);

        // Draw everything here...

        // Display what has been drawn to the window
        window.display();
    }
