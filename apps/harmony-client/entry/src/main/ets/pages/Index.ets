import { HelloAck } from 'libentry.so';
import { LinkScreenConnection } from '../services/LinkScreenConnection';

@Entry
@Component
struct Index {
  @State private hostAddress: string = '127.0.0.1';
  @State private statusText: string = '尚未连接';
  @State private sessionText: string = '';
  private connection: LinkScreenConnection = new LinkScreenConnection(
    (status: string) => {
      this.statusText = status;
    },
    (ack: HelloAck) => {
      this.sessionText = `会话 ${ack.sessionId.toString()} · 能力 0x${ack.selectedCapabilities.toString(16)}`;
    }
  );

  aboutToDisappear(): void {
    this.connection.close();
  }

  build() {
    Column({ space: 20 }) {
      Text('灵犀副屏')
        .fontSize(32)
        .fontWeight(FontWeight.Bold)
        .fontColor('#0F172A')

      Text('将 HarmonyOS 平板连接为 Windows 可交互副屏')
        .fontSize(16)
        .fontColor('#64748B')

      TextInput({ text: this.hostAddress, placeholder: 'Windows 主机地址' })
        .type(InputType.Normal)
        .height(52)
        .onChange((value: string) => {
          this.hostAddress = value;
        })

      Button('连接 Windows 主机')
        .width('100%')
        .height(52)
        .onClick(() => {
          this.sessionText = '';
          this.connection.connect(this.hostAddress.trim());
        })

      Text(this.statusText)
        .fontSize(15)
        .fontColor('#334155')
        .width('100%')

      if (this.sessionText.length > 0) {
        Text(this.sessionText)
          .fontSize(13)
          .fontColor('#2563EB')
          .width('100%')
      }

      Text('安全提示：完成配对和加密前，Windows 开发主机仅监听本机回环地址。')
        .fontSize(13)
        .fontColor('#94A3B8')
    }
    .width('100%')
    .height('100%')
    .padding(32)
    .justifyContent(FlexAlign.Center)
    .backgroundColor('#F8FAFC')
  }
}
