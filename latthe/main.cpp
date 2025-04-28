#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <random>

#define BACK_CARD "assets//entity//logo_mlbb.jpg"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int CARD_WIDTH = 100;
const int CARD_HEIGHT = 100;
const int NUM_PAIRS = 6;
const int NUM_COLUMNS = 4;

struct Card {
    SDL_Rect rect;
    int id;
    bool revealed = false;
    bool matched = false;
};

SDL_Texture* loadTexture(const std::string& path, SDL_Renderer* renderer) {
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if (!loadedSurface) {
        std::cerr << "Failed to load image: " << path << " - " << IMG_GetError() << "\n";
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
    SDL_FreeSurface(loadedSurface);
    return texture;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << "\n";
        return -1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "Failed to initialize SDL_image: " << IMG_GetError() << "\n";
        SDL_Quit();
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Card Flip Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << "\n";
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Texture* backTexture = loadTexture(BACK_CARD, renderer);
    if (!backTexture) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    std::vector<SDL_Texture*> cardTextures;
    for (int i = 0; i < NUM_PAIRS; ++i) {
        std::string path = "assets//entity//anh_" + std::to_string(i) + ".jpg";
        SDL_Texture* tex = loadTexture(path, renderer);
        if (!tex) {
            std::cerr << "Failed to load card texture: " << path << "\n";
            // Giải phóng bộ nhớ trước khi thoát
            for (auto& t : cardTextures) SDL_DestroyTexture(t);
            SDL_DestroyTexture(backTexture);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            IMG_Quit();
            SDL_Quit();
            return -1;
        }
        cardTextures.push_back(tex);
    }

    std::vector<int> ids;
    for (int i = 0; i < NUM_PAIRS; ++i) {
        ids.push_back(i);
        ids.push_back(i);
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(ids.begin(), ids.end(), g);

    std::vector<Card> cards;
    for (int i = 0; i < ids.size(); ++i) {
        Card card;
        card.id = ids[i];
        card.rect = {
            50 + (i % NUM_COLUMNS) * (CARD_WIDTH + 10),
            60 + (i / NUM_COLUMNS) * (CARD_HEIGHT + 10),
            CARD_WIDTH, CARD_HEIGHT
        };
        cards.push_back(card);
    }

    int revealedCount = 0;
    int firstIndex = -1, secondIndex = -1;
    Uint32 revealTime = 0;
    bool quit = false;
    bool win = false;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                quit = true;

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && revealedCount < 2 && !win) {
                int x = e.button.x;
                int y = e.button.y;

                for (int i = 0; i < cards.size(); ++i) {
                    SDL_Point point = {x, y};
                    if (!cards[i].revealed && !cards[i].matched &&
                        SDL_PointInRect(&point, &cards[i].rect)) {
                        cards[i].revealed = true;
                        if (revealedCount == 0) {
                            firstIndex = i;
                            revealedCount = 1;
                        } else if (revealedCount == 1) {
                            secondIndex = i;
                            revealedCount = 2;
                            revealTime = SDL_GetTicks();
                        }
                        break;
                    }
                }
            }
        }

        if (revealedCount == 2 && SDL_GetTicks() - revealTime > 1000) {
            if (cards[firstIndex].id == cards[secondIndex].id) {
                cards[firstIndex].matched = true;
                cards[secondIndex].matched = true;
            } else {
                cards[firstIndex].revealed = false;
                cards[secondIndex].revealed = false;
            }
            revealedCount = 0;
            firstIndex = secondIndex = -1;
        }

        if (!win) {
            bool allMatched = true;
            for (const auto& card : cards) {
                if (!card.matched) {
                    allMatched = false;
                    break;
                }
            }
            if (allMatched) {
                win = true;
                std::cout << "You Win!" << std::endl;
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        for (int i = 0; i < cards.size(); ++i) {
            SDL_Texture* texture = backTexture;
            if (cards[i].revealed || cards[i].matched) {
                texture = cardTextures[cards[i].id];
            }
            SDL_RenderCopy(renderer, texture, nullptr, &cards[i].rect);
        }

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    for (auto& tex : cardTextures) SDL_DestroyTexture(tex);
    SDL_DestroyTexture(backTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
