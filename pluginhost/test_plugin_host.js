const { PluginHost } = require('./index');

// Test function to verify plugin host functionality
async function testPluginHost() {
  console.log('Starting Plugin Host test...');
  
  try {
    // Initialize the plugin host
    const pluginHost = new PluginHost();
    console.log('Plugin Host initialized successfully');
    
    // Test loading available plugins
    const plugins = await pluginHost.getAvailablePlugins();
    console.log('Available plugins:', plugins);
    
    // Test connection to Voicemeeter if applicable
    if (typeof pluginHost.connect === 'function') {
      await pluginHost.connect();
      console.log('Plugin Host connected to Voicemeeter');
      
      // Test getting parameters if available
      if (typeof pluginHost.getParameters === 'function') {
        const params = await pluginHost.getParameters();
        console.log('Plugin parameters:', params);
      }
      
      // Disconnect if connected
      if (typeof pluginHost.disconnect === 'function') {
        await pluginHost.disconnect();
        console.log('Plugin Host disconnected from Voicemeeter');
      }
    }
    
    console.log('Plugin Host test completed successfully');
  } catch (error) {
    console.error('Plugin Host test failed:', error);
  }
}

// Run the test
testPluginHost();
