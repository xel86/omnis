export const UNITS = ['B', 'KB', 'MB', 'GB'];

export function convertToUnit(bytes: number, unitIndex: number): number {
  switch (unitIndex) {
    case 0:
      return bytes;
    case 1:
      return bytes / 1000;
    case 2:
      return bytes / 1000000;
    case 3:
      return bytes / 1000000000;
    default:
      return bytes;
  }
}
