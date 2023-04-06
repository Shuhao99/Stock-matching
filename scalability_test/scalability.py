import xml.etree.ElementTree as eTree
import socket
import time


def createXml(id, balance, symbols):
    data = eTree.Element("create")
    eTree.SubElement(data, "account", id=str(id), balance=balance)
    for symbol, amount in symbols.items():
        sym = eTree.SubElement(data, "symbol", sym=symbol)
        account = eTree.SubElement(sym, "account", id=str(id))
        account.text = amount
    return formatXmlData(data)


def orderXml(id, order):
    data = eTree.Element("transactions", id=str(id))
    for symbol, amount, limit in order:
        eTree.SubElement(data, "order", sym=symbol, amount=str(amount), limit=str(limit))
    return formatXmlData(data)


def queryXml(id, query):
    data = eTree.Element("transactions", id=str(id))
    for transactionId in query:
        eTree.SubElement(data, "query", id=transactionId)
    return formatXmlData(data)


def cancelXml(id, cancel):
    data = eTree.Element("transactions", id=str(id))
    for transactionId in cancel:
        eTree.SubElement(data, "cancel", id=transactionId)
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
    client.sendall(createXml(str(id), balance, symbols))
    printResponse(client)


def orderRequest(id, order):
    client = createClient()
    data = orderXml(id, order)
    client.sendall(data)
    printResponse(client)


def queryRequest(id, query):
    client = createClient()
    data = queryXml(id, query)
    client.sendall(data)
    printResponse(client)


def cancelRequest(id, cancel):
    client = createClient()
    data = cancelXml(id, cancel)
    client.sendall(data)
    printResponse(client)


def printResponse(client):
    try:
        # print(client.recv(99999))
        client.close()
    except Exception as e:
        print(e)
        client.close()
        exit(1)


def test(iterations):
    count = 0
    startTime = int(1000 * time.time())
    while count < iterations:
        # perfect match
        createRequest(1, "99999", {"A": "100", "B": "200"})
        createRequest(2, "99999", {"A": "200", "B": "100"})
        orderRequest(1, [("A", 50, 100)])
        orderRequest(2, [("A", -50, 100)])
        queryRequest(1, ["1"])
        queryRequest(2, ["0"])
        cancelRequest(1, ["0"])
        cancelRequest(2, ["1"])
        # perfect match
        createRequest(3, "99999", {"C": "100", "D": "200"})
        createRequest(4, "99999", {"C": "200", "D": "100"})
        orderRequest(3, [("C", 50, 100)])
        orderRequest(4, [("C", -40, 100)])
        queryRequest(3, ["2"])
        queryRequest(4, ["3"])
        cancelRequest(3, ["2"])
        cancelRequest(4, ["3"])
        # perfect match
        createRequest(5, "99999", {"E": "100", "F": "200"})
        createRequest(6, "99999", {"E": "200", "F": "100"})
        orderRequest(5, [("E", 50, 100)])
        orderRequest(6, [("E", -50, 30)])
        queryRequest(5, ["4"])
        queryRequest(6, ["5"])
        cancelRequest(5, ["4"])
        cancelRequest(6, ["5"])
        # perfect match
        createRequest(7, "99999", {"G": "100", "H": "200"})
        createRequest(8, "99999", {"G": "200", "H": "100"})
        orderRequest(7, [("G", 50, 100)])
        orderRequest(8, [("G", -30, 20)])
        queryRequest(7, ["6"])
        queryRequest(8, ["7"])
        cancelRequest(7, ["6"])
        cancelRequest(8, ["7"])
        # perfect match
        createRequest(9, "99999", {"I": "100", "J": "200"})
        createRequest(10, "99999", {"I": "200", "J": "100"})
        orderRequest(9, [("I", 50, 100)])
        orderRequest(10, [("I", -60, 20)])
        queryRequest(9, ["8"])
        queryRequest(10, ["9"])
        cancelRequest(9, ["8"])
        cancelRequest(10, ["9"])
        # perfect match
        createRequest(11, "99999", {"K": "100", "L": "200"})
        createRequest(12, "99999", {"K": "200", "L": "100"})
        orderRequest(11, [("K", -30, 30)])
        orderRequest(12, [("K", 60, 50)])
        queryRequest(11, ["10"])
        queryRequest(12, ["11"])
        cancelRequest(11, ["10"])
        cancelRequest(12, ["11"])
        # perfect match
        createRequest(13, "99999", {"M": "100", "N": "200"})
        createRequest(14, "99999", {"M": "200", "N": "100"})
        orderRequest(13, [("M", -60, 30)])
        orderRequest(14, [("M", 30, 50)])
        queryRequest(13, ["12"])
        queryRequest(14, ["13"])
        cancelRequest(13, ["12"])
        cancelRequest(14, ["13"])
        # none match
        createRequest(15, "99999", {"O": "100", "P": "200"})
        createRequest(16, "99999", {"O": "200", "P": "100"})
        orderRequest(15, [("O", -60, 100)])
        orderRequest(16, [("O", 30, 20)])
        queryRequest(15, ["14"])
        queryRequest(16, ["15"])
        cancelRequest(15, ["14"])
        cancelRequest(16, ["15"])
        # none match
        createRequest(17, "99999", {"Q": "100", "R": "200"})
        createRequest(18, "99999", {"Q": "200", "R": "100"})
        orderRequest(17, [("Q", 50, 20)])
        orderRequest(18, [("Q", -60, 50)])
        queryRequest(17, ["16"])
        queryRequest(18, ["17"])
        cancelRequest(17, ["16"])
        cancelRequest(18, ["17"])
        # group match
        createRequest(19, "99999", {"S": "100", "T": "200"})
        createRequest(20, "99999", {"S": "200", "T": "100"})
        createRequest(21, "99999", {"S": "200", "T": "100"})
        orderRequest(19, [("S", -50, 30)])
        orderRequest(20, [("S", -50, 20)])
        orderRequest(21, [("S", 100, 50)])
        queryRequest(19, ["18"])
        queryRequest(20, ["19"])
        queryRequest(21, ["20"])
        # group match
        createRequest(22, "99999", {"U": "100", "V": "200"})
        createRequest(23, "99999", {"U": "200", "V": "100"})
        createRequest(24, "99999", {"U": "200", "V": "100"})
        orderRequest(22, [("U", -50, 30)])
        orderRequest(23, [("U", -50, 20)])
        orderRequest(24, [("U", 80, 50)])
        queryRequest(22, ["21"])
        queryRequest(23, ["22"])
        queryRequest(24, ["23"])
        # group match
        createRequest(25, "99999", {"W": "100", "X": "200"})
        createRequest(26, "99999", {"W": "200", "X": "100"})
        createRequest(27, "99999", {"W": "200", "X": "100"})
        orderRequest(25, [("W", 50, 30)])
        orderRequest(26, [("W", 50, 20)])
        orderRequest(27, [("W", -100, 10)])
        queryRequest(25, ["24"])
        queryRequest(26, ["25"])
        queryRequest(27, ["26"])
        # group match
        createRequest(28, "99999", {"Y": "100", "Z": "200"})
        createRequest(29, "99999", {"Y": "200", "Z": "100"})
        createRequest(30, "99999", {"Y": "200", "Z": "100"})
        orderRequest(28, [("Y", 50, 30)])
        orderRequest(29, [("Y", 50, 20)])
        orderRequest(30, [("Y", -80, 10)])
        queryRequest(28, ["27"])
        queryRequest(29, ["28"])
        queryRequest(30, ["29"])
        # sell not owned
        createRequest(31, "99999", {"AA": "100", "BB": "200"})
        createRequest(32, "99999", {"AA": "200", "BB": "100"})
        createRequest(33, "99999", {"AA": "200", "BB": "100"})
        orderRequest(31, [("AA", -50, 30)])
        orderRequest(32, [("A", -50, 20)])
        orderRequest(33, [("A", 100, 50)])
        queryRequest(31, ["30"])
        queryRequest(32, ["31"])
        queryRequest(33, ["32"])
        count = count + 1
    endTime = int(1000 * time.time())
    print("Total Time with iterations of " + str(iterations) + " is " + str(endTime - startTime))


if __name__ == "__main__":
    test(10)
    test(20)
    test(30)
    test(40)
    test(50)
