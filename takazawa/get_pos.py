# -*- coding: utf-8 -*-

import cv2

# 'r' で画像更新
# 画像をクリックするとその位置をprint
# esc or 右クリックで終了
# pos.txtに最小x,yを保存するようにmain.cppを書き換えが必要

class mouseParam:
    def __init__(self, input_img_name):
        #マウス入力用のパラメータ
        self.mouseEvent = {"x":None, "y":None, "event":None, "flags":None}
        #マウス入力の設定
        cv2.setMouseCallback(input_img_name, self.__CallBackFunc, None)
    
    #コールバック関数
    def __CallBackFunc(self, eventType, x, y, flags, userdata):
        
        self.mouseEvent["x"] = x
        self.mouseEvent["y"] = y
        self.mouseEvent["event"] = eventType    
        self.mouseEvent["flags"] = flags    

    #マウス入力用のパラメータを返すための関数
    def getData(self):
        return self.mouseEvent
    
    #マウスイベントを返す関数
    def getEvent(self):
        return self.mouseEvent["event"]                

    #マウスフラグを返す関数
    def getFlags(self):
        return self.mouseEvent["flags"]                

    #xの座標を返す関数
    def getX(self):
        return self.mouseEvent["x"]  

    #yの座標を返す関数
    def getY(self):
        return self.mouseEvent["y"]  

    #xとyの座標を返す関数
    def getPos(self):
        return (self.mouseEvent["x"], self.mouseEvent["y"])
        

if __name__ == "__main__":
    read = cv2.imread("output.pnm")
    pos = ()
    with open('pos.txt') as f:
        pos = list(map(int, f.readline().split()))
        print(pos)
    
    #表示するWindow名
    window_name = "input window"
    
    #画像の表示
    cv2.imshow(window_name, read)
    
    #コールバックの設定
    mouseData = mouseParam(window_name)
    
    while 1:
        cv2.waitKey(20)
        #左クリックがあったら表示
        if mouseData.getEvent() == cv2.EVENT_LBUTTONDOWN:
            x, y = mouseData.getPos()
            if pos != ():
                print(x + pos[0], y + pos[1]) 
        #右クリックがあったら終了
        elif mouseData.getEvent() == cv2.EVENT_RBUTTONDOWN:
            break
        if cv2.waitKey(20) & 0xFF == ord('r'):
            print('update')
            read = cv2.imread("output.pnm")
            cv2.imshow(window_name, read)
            pos = ()
            with open('pos.txt') as f:
                pos = list(map(int, f.readline().split()))
                print(pos)

    cv2.destroyAllWindows()            
    print("Finished")