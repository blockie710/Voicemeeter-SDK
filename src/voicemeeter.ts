import { VoicemeeterRemote } from './voicemeeter-remote';

export class Voicemeeter {
  private remote: VoicemeeterRemote;
  private isConnected: boolean;

  constructor() {
    this.remote = new VoicemeeterRemote();
    this.isConnected = false;
  }

  async login(): Promise<void> {
    try {
      await this.remote.login();
      this.isConnected = true;
    } catch (error) {
      throw new Error(`Failed to login to Voicemeeter: ${error.message}`);
    }
  }

  async logout(): Promise<void> {
    try {
      await this.remote.logout();
      this.isConnected = false;
    } catch (error) {
      throw new Error(`Failed to logout from Voicemeeter: ${error.message}`);
    }
  }

  setParameterFloat(paramName: string, value: number): void {
    if (typeof paramName !== 'string' || paramName.trim() === '') {
      throw new Error('Parameter name must be a non-empty string');
    }

    if (typeof value !== 'number' || isNaN(value)) {
      throw new Error('Value must be a valid number');
    }

    this.remote.setParameterFloat(paramName, value);
  }

  getParameterFloat(paramName: string): number {
    if (typeof paramName !== 'string' || paramName.trim() === '') {
      throw new Error('Parameter name must be a non-empty string');
    }

    return this.remote.getParameterFloat(paramName);
  }
}