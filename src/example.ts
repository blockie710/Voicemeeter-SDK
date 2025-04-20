/**
 * Example usage of the Voicemeeter SDK
 */

import { VoicemeeterClient } from './voicemeeter';

async function example() {
  // Create a Voicemeeter client
  const voicemeeter = new VoicemeeterClient({ 
    type: 'banana',
    autoConnect: true,
    pollingInterval: 100
  });

  try {
    // Login to Voicemeeter
    await voicemeeter.login();
    console.log('Connected to Voicemeeter Banana');

    // Set input strip 0 gain to -10 dB
    voicemeeter.setParameterFloat('Strip[0].Gain', -10);
    
    // Get current gain of input strip 0
    const gain = voicemeeter.getParameterFloat('Strip[0].Gain');
    console.log(`Current gain of input strip 0: ${gain} dB`);

    // Mute input strip 0
    voicemeeter.setParameterFloat('Strip[0].Mute', 1);
    
    // Set label of input strip 0
    voicemeeter.setParameterString('Strip[0].Label', 'Microphone');

    // Wait for 5 seconds
    await new Promise(resolve => setTimeout(resolve, 5000));
    
    // Logout from Voicemeeter
    await voicemeeter.logout();
    console.log('Disconnected from Voicemeeter');
  } catch (error) {
    console.error('Error:', error.message);
  }
}

// Run the example if this file is executed directly
if (require.main === module) {
  example();
}
