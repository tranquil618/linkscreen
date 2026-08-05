import { socket } from '@kit.NetworkKit';
import linkscreen, { HelloAck } from 'libentry.so';

const CONTROL_PORT: number = 47831;
const CLIENT_CAPABILITIES: number = 0x0F;

export type StatusListener = (status: string) => void;
export type ConnectedListener = (ack: HelloAck) => void;

export class LinkScreenConnection {
  private client: socket.TCPSocket | undefined = undefined;
  private receiveBuffer: Uint8Array = new Uint8Array(0);
  private handshakeComplete: boolean = false;
  private readonly onStatus: StatusListener;
  private readonly onConnected: ConnectedListener;

  constructor(onStatus: StatusListener, onConnected: ConnectedListener) {
    this.onStatus = onStatus;
    this.onConnected = onConnected;
  }

  async connect(hostAddress: string): Promise<void> {
    await this.close();
    this.receiveBuffer = new Uint8Array(0);
    this.handshakeComplete = false;
    this.onStatus('正在连接 Windows 主机…');

    const client = socket.constructTCPSocketInstance();
    this.client = client;
    client.on('message', (info: socket.SocketMessageInfo) => {
      this.handleMessage(info.message);
    });
    client.on('error', (error: Error) => {
      this.onStatus(`网络错误：${error.message}`);
    });
    client.on('close', () => {
      if (!this.handshakeComplete) {
        this.onStatus('连接已关闭');
      }
      this.client = undefined;
    });

    try {
      await client.bind({ address: '0.0.0.0', family: 1, port: 0 });
      await client.connect({
        address: { address: hostAddress, family: 1, port: CONTROL_PORT },
        timeout: 5000
      });

      const helloFrame: ArrayBuffer = linkscreen.encodeHelloFrame(
        'HarmonyOS 6.1 Tablet',
        CLIENT_CAPABILITIES,
        1
      );
      await client.send({ data: helloFrame });
      this.onStatus('已发送 Hello，等待主机确认…');
    } catch (error) {
      this.onStatus(`连接失败：${JSON.stringify(error)}`);
      await this.close();
    }
  }

  async close(): Promise<void> {
    const current = this.client;
    this.client = undefined;
    if (current !== undefined) {
      current.off('message');
      current.off('error');
      current.off('close');
      try {
        await current.close();
      } catch (_) {
        // Closing an already-closed socket is harmless during reconnect.
      }
    }
  }

  private appendChunk(chunk: ArrayBuffer): void {
    const incoming = new Uint8Array(chunk);
    const combined = new Uint8Array(this.receiveBuffer.byteLength + incoming.byteLength);
    combined.set(this.receiveBuffer, 0);
    combined.set(incoming, this.receiveBuffer.byteLength);
    this.receiveBuffer = combined;
  }

  private handleMessage(chunk: ArrayBuffer): void {
    try {
      this.appendChunk(chunk);
      const expectedSize: number = linkscreen.frameSizeFromPrefix(this.receiveBuffer.buffer);
      if (expectedSize === 0 || this.receiveBuffer.byteLength < expectedSize) {
        return;
      }
      if (this.receiveBuffer.byteLength !== expectedSize) {
        throw new Error('握手阶段收到了多余数据');
      }

      const ack: HelloAck = linkscreen.decodeHelloAckFrame(this.receiveBuffer.buffer);
      this.receiveBuffer = new Uint8Array(0);
      if (ack.status !== 0) {
        this.onStatus(`主机拒绝连接，状态码：${ack.status}`);
        return;
      }

      this.handshakeComplete = true;
      this.onConnected(ack);
      this.onStatus(`已连接到 ${ack.hostName}`);
    } catch (error) {
      this.onStatus(`响应解析失败：${JSON.stringify(error)}`);
    }
  }
}
