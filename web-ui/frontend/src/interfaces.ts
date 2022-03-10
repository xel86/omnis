export interface Application {
  id: number;
  name: string;
}

export interface Session {
  start: number;
  durationSec: number;
  applicationId: number;
  bytesTx: number;
  bytesRx: number;
  pktTx: number;
  pktRx: number;
  pktTcp: number;
  pktUdp: number;
}

export interface AppSession {
  application: Application;
  sessions: Session[];
}

export interface SessionTotal {
  bytesTx: number;
  bytesRx: number;
  pktTx: number;
  pktRx: number;
  pktTcp: number;
  pktUdp: number;
}
