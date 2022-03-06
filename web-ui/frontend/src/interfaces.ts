export interface Application {
  id: number;
  name: string;
}

export interface Session {
  start: Date;
  durationSec: number;
  applicationID: number;
  bytes: number;
  pktTx: number;
  pktRx: number;
  pktTotal: number;
  pktTcp: number;
  pktUdp: number;
}

export interface AppSession {
  application: Application;
  sessions: Session[];
}
