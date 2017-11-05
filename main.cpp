#include <iostream>
#include <SDL2/SDL.h>

//The window we'll be rendering to
SDL_Window* gWindow = NULL;
    
//The surface contained by the window
SDL_Surface* gScreenSurface = NULL;

//The image we will load and show on the screen
uint8_t* gPixels = NULL;
SDL_Surface* gHelloWorld = NULL;

extern "C" {
extern const uint32_t g_palette[];
}

//Starts up SDL and creates window
bool init(int width, int height)
{
    //Initialization flag
    bool success = true;

    //Initialize SDL
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
        success = false;
    }
    else
    {
        //Create window
        gWindow = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN );
        if( gWindow == NULL )
        {
            printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
            success = false;
        }
        else
        {
            //Get window surface
            gScreenSurface = SDL_GetWindowSurface( gWindow );
            SDL_SetSurfaceBlendMode(gScreenSurface, SDL_BLENDMODE_BLEND);
        }
    }

    return success;
}

uint8_t* readFile(const char* filename, long offset, size_t bytes)
{
    FILE* file = fopen(filename, "rb");
    if (file) {
        printf("opened '%s', seeking to 0x%08lX\n", filename, offset);
        if (fseek(file, offset, SEEK_SET) == 0) {
            uint8_t* pixels = static_cast<uint8_t*>(malloc(bytes));
            if (pixels) {
                printf("reading %zu bytes\n", bytes);
                size_t bytes_read = fread(pixels, 1, bytes, file);
                if (bytes_read == bytes) {
                    printf("file read, closing file\n");
                    fclose(file);
                    return pixels;
                }
                free(pixels);
            }
        }
        fclose(file);
    }
    return NULL;
}

uint8_t c6to8bit(int v)
{
    return (v * 255) / 63;
}


uint8_t c8to6bit(int v)
{
    return v >> 2;
}

uint32_t map_rgb(uint32_t rgb)
{
    uint8_t r = c6to8bit((rgb >> 16) & 0xFF);
    uint8_t g = c6to8bit((rgb >> 8) & 0xFF);
    uint8_t b = c6to8bit(rgb & 0xFF);
    return SDL_MapRGB(gScreenSurface->format, r, g, b);
}

//Loads media
bool loadMedia(const char* filename, long offset, int width, int height)
{
    //Loading success flag
    bool success = false;

    gPixels = readFile(filename, offset, width * height);
    if (gPixels) {
        printf("loaded pixels, creating surface\n");
        gHelloWorld = SDL_CreateRGBSurfaceFrom(gPixels, width, height, 8, width, 0, 0, 0, 0);
        if (gHelloWorld) {
            printf("surface created, changing palette\n");
            SDL_SetSurfaceBlendMode(gHelloWorld, SDL_BLENDMODE_NONE);
            SDL_LockSurface(gHelloWorld);
            SDL_Palette* palette = gHelloWorld->format->palette;
            if (palette) {
                SDL_Color colors[256];
#if 0
                uint8_t* c_palette = readFile("/tmp/palette.rgb", 0, 0x300);
                for (int c = 0; c < 256; ++c) {
                    colors[c].r = c6to8bit(c_palette[c * 3 + 0]);
                    colors[c].g = c6to8bit(c_palette[c * 3 + 1]);
                    colors[c].b = c6to8bit(c_palette[c * 3 + 2]);
                }
                free(c_palette);
#else
                for (int c = 0; c < 256; ++c) {
                    uint32_t rgb = g_palette[c];
                    colors[c].r = (rgb >> 16) & 0xFF;
                    colors[c].g = (rgb >> 8) & 0xFF;
                    colors[c].b = rgb & 0xFF;
                }
#endif
                printf("setting palette\n");
                SDL_SetPaletteColors(palette, colors, 0, 256);
            }
            SDL_UnlockSurface(gHelloWorld);
            SDL_Surface* optimized_surface = SDL_ConvertSurface(gHelloWorld, gScreenSurface->format, 0);
            if (optimized_surface) {
                SDL_FreeSurface(gHelloWorld);
                gHelloWorld = optimized_surface;
            }
            success = true;
        }
        else {
            printf("no surface\n");
        }
    }

    return success;
}

//Frees media and shuts down SDL
void close()
{
    //Deallocate surface
    SDL_FreeSurface( gHelloWorld );
    gHelloWorld = NULL;
    free(gPixels);
    gPixels = NULL;

    //Destroy window
    SDL_DestroyWindow( gWindow );
    gWindow = NULL;

    //Quit SDL subsystems
    SDL_Quit();
}

int main(int argc, char* argv[]){
    if (argc != 5) {
        printf("%s: <file> <offset> <width> <height>\n", basename(argv[0]));
        return -1;
    }

    const char* filename = argv[1];
    const long offset = strtol(argv[2], NULL, 0);
    const int width = strtol(argv[3], NULL, 0);
    const int height = strtol(argv[4], NULL, 0);

    //Start up SDL and create window
    if( !init(width * 5, height * 5) )
    {
        printf( "Failed to initialize!\n" );
    }
    else
    {
        //Load media
        if( !loadMedia(filename, offset, width, height) )
        {
            printf( "Failed to load media!\n" );
        }
        else
        {
            //Apply the image
            int rv = SDL_BlitScaled( gHelloWorld, NULL, gScreenSurface, NULL );
            if (rv) {
                printf("blit failed: %s\n", SDL_GetError());
            }
//            SDL_BlitSurface( gHelloWorld, NULL, gScreenSurface, NULL );
            //Update the surface
            SDL_UpdateWindowSurface( gWindow );
            //Wait two seconds
            SDL_Delay( 2000 );
        }
    }

    //Free resources and close SDL
    close();

    return 0;
}

