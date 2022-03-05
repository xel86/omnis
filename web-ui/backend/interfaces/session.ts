export interface Session {
  start: Date;
  duration: number;
  applicationID: number;
  bytes: number;
  pktTx: number;
  pktRx: number;
  pktTotal: number;
  pktTcp: number;
  pktUdp: number;
}
