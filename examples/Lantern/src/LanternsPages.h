#ifndef LANTERNSPAGES_H
#define LANTERNSPAGES_H

#include <CustomPageHandler.h>
#include <ConfigServer.h>
#include <WebServer.h>

/**
 * @brief Example custom page handler for Lanterns functionality
 * 
 * This is an example implementation showing how to create custom web pages
 * for your device's primary functionality while keeping the code separate
 * from the EasyWiFi library.
 * 
 * Features demonstrated:
 * - Registering custom routes (/lanterns, /api/lanterns/*)
 * - Using WebPages helpers for consistent styling
 * - Accessing WiFi status and storage
 * - Creating both web UI and REST API endpoints
 * 
 * To customize for your project:
 * 1. Rename this class to match your device (e.g., SmartPlugPages)
 * 2. Update the route URLs (e.g., /plug instead of /lanterns)
 * 3. Implement your device-specific functionality
 * 4. Add your custom HTML/controls in the handler methods
 */
class LanternsPages : public CustomPageHandler {
public:
    /**
     * @brief Constructor
     * @param configServer Reference to ConfigServer for route registration
     */
    LanternsPages(ConfigServer& configServer);
    
    /**
     * @brief Register all custom routes with the web server
     * 
     * Called automatically by ConfigServer.registerCustomHandler()
     */
    void registerRoutes() override;
    
private:
    ConfigServer& configServer;
    
    // Web UI route handlers
    void handleLanternsHome();
    void handleLanternsControl();
    void handleLanternsSettings();
    
    // REST API route handlers
    void handleAPILanternsStatus();
    void handleAPILanternsControl();
    
    // Helper methods
    String getLanternStatusHTML();
};

#endif // LANTERNSPAGES_H

