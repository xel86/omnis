/*
  Warnings:

  - The primary key for the `Session` table will be changed. If it partially fails, the table could be left without primary key constraint.
  - You are about to drop the column `applicationID` on the `Session` table. All the data in the column will be lost.
  - You are about to drop the column `bytes` on the `Session` table. All the data in the column will be lost.
  - You are about to alter the column `start` on the `Session` table. The data in that column could be lost. The data in that column will be cast from `DateTime` to `Int`.
  - Added the required column `applicationId` to the `Session` table without a default value. This is not possible if the table is not empty.
  - Added the required column `bytesRx` to the `Session` table without a default value. This is not possible if the table is not empty.
  - Added the required column `bytesTx` to the `Session` table without a default value. This is not possible if the table is not empty.

*/
-- RedefineTables
PRAGMA foreign_keys=OFF;
CREATE TABLE "new_Session" (
    "start" INTEGER NOT NULL,
    "applicationId" INTEGER NOT NULL,
    "durationSec" INTEGER NOT NULL,
    "bytesTx" INTEGER NOT NULL,
    "bytesRx" INTEGER NOT NULL,
    "pktTx" INTEGER NOT NULL,
    "pktRx" INTEGER NOT NULL,
    "pktTcp" INTEGER NOT NULL,
    "pktUdp" INTEGER NOT NULL,

    PRIMARY KEY ("start", "applicationId"),
    CONSTRAINT "Session_applicationId_fkey" FOREIGN KEY ("applicationId") REFERENCES "Application" ("id") ON DELETE RESTRICT ON UPDATE CASCADE
);
INSERT INTO "new_Session" ("durationSec", "pktRx", "pktTcp", "pktTx", "pktUdp", "start") SELECT "durationSec", "pktRx", "pktTcp", "pktTx", "pktUdp", "start" FROM "Session";
DROP TABLE "Session";
ALTER TABLE "new_Session" RENAME TO "Session";
PRAGMA foreign_key_check;
PRAGMA foreign_keys=ON;
