#include <SFML/Graphics.hpp>

namespace UI
{
    int RunUI() {
        // Create an 1800x1800 pixel window
        sf::RenderWindow window(sf::VideoMode(1200, 1200), "6x6 Pixel Grid with Padding", sf::Style::Titlebar | sf::Style::Close);

        // Define grid cell size and padding
        const int cellSize = 4;
        const int padding = 50;

        // Calculate the number of cells that fit within the padded area
        const int gridWidth = (1200 - 2 * padding) / cellSize;
        const int gridHeight = (1200c - 2 * padding) / cellSize;

        // Main loop
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    window.close();
            }

            window.clear(sf::Color::White);

            // Draw the grid with padding
            for (int x = 0; x < gridWidth; ++x) {
                for (int y = 0; y < gridHeight; ++y) {
                    sf::RectangleShape cell(sf::Vector2f(cellSize, cellSize));

                    // Set cell position with padding
                    cell.setPosition(x * cellSize + padding, y * cellSize + padding);

                    // Set cell color and transparent border
                    cell.setFillColor(sf::Color::White);
                    cell.setOutlineThickness(1);
                    cell.setOutlineColor(sf::Color(0, 0, 0, 128)); // RGBA with 128 alpha for transparency

                    window.draw(cell);
                }
            }

            window.display();
        }

        return 0;
    }
}
