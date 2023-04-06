import xml.etree.ElementTree as eTree
import socket
import time
from multiprocessing import Process


def createXml(id, balance, symbols):
    data = eTree.Element("create")
    eTree.SubElement(data, "account", id=str(id), balance=balance)
    for symbol, amount in symbols.items():
        sym = eTree.SubElement(data, "symbol", sym=symbol)
        account = eTree.SubElement(sym, "account", id=str(id))
        account.text = amount
    return formatXmlData(data)


def formatXmlData(data):
    dataString = eTree.tostring(data)
    dataString = str(len(dataString)) + "\n" + dataString.decode("utf8", "strict")
    return dataString.encode("utf8", "strict")


def createClient():
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect(("vcm-30745.vm.duke.edu", 12345))
    return client


def createRequest(id, balance, symbols):
    client = createClient()
    client.sendall(createXml(id, balance, symbols))
    printResponse(client)


def printResponse(client):
    try:
        # print(client.recv(10000))
        client.close()
    except Exception as e:
        print(e)
        client.close()
        exit(1)


def measureTime(data):
    id = 0
    while id < 1000:
        createRequest(str(data + id), "99999", {"SPY": "500", "BTC": "600"})
        id = id + 1


if __name__ == "__main__":
    startTime = int(1000 * time.time())

    processes = []
    for i in range(10):
        p = Process(args=(i * 1000,), target=measureTime)
        p.start()
        processes.append(p)

    for p in processes:
        p.join()

    endTime = int(1000 * time.time())
    print("Total Time: " + str(endTime - startTime))
    print("Throughput (query/ms): " + str(10000 / (endTime - startTime)))
