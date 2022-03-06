-- CreateTable
CREATE TABLE "Application" (
    "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    "name" TEXT NOT NULL
);

-- CreateTable
CREATE TABLE "Session" (
    "start" DATETIME NOT NULL,
    "applicationID" INTEGER NOT NULL,
    "durationSec" INTEGER NOT NULL,
    "bytes" INTEGER NOT NULL,
    "pktTx" INTEGER NOT NULL,
    "pktRx" INTEGER NOT NULL,
    "pktTcp" INTEGER NOT NULL,
    "pktUdp" INTEGER NOT NULL,

    PRIMARY KEY ("start", "applicationID"),
    CONSTRAINT "Session_applicationID_fkey" FOREIGN KEY ("applicationID") REFERENCES "Application" ("id") ON DELETE RESTRICT ON UPDATE CASCADE
);

-- CreateIndex
CREATE UNIQUE INDEX "Application_name_key" ON "Application"("name");
