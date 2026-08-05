export interface HelloAck {
  status: number;
  selectedVersion: number;
  selectedCapabilities: number;
  sessionId: bigint;
  hostName: string;
}

export interface LinkScreenNative {
  encodeHelloFrame(
    deviceName: string,
    capabilities: number,
    sequence: number
  ): ArrayBuffer;

  decodeHelloAckFrame(frame: ArrayBuffer): HelloAck;

  // Returns 0 until a complete 16-byte header is available. Once the header is
  // present, returns the exact total frame size or throws for an invalid header.
  frameSizeFromPrefix(bytes: ArrayBuffer): number;
}

declare const linkscreen: LinkScreenNative;
export default linkscreen;
