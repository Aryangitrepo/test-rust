
use axum::extract::ws::{ self, WebSocket};
use futures_util::{stream::{SplitSink}, SinkExt, StreamExt};
use tokio::sync::mpsc::{
    self, Receiver
};

use crate::{utils::mqtt};



pub async fn handle_socket(socket: WebSocket) {
    let (sender,_) = socket.split();
    
    let (tx, rx)=mpsc::channel::<String>(30);


        mqtt::spawn_mqtt(tx);
        tokio::spawn(write(sender,rx));
        
}
pub async fn write(mut sender:SplitSink<WebSocket,ws::Message>,mut rx:Receiver<String>){
   while let Some(data)=rx.recv().await{
        sender.send(ws::Message::from(data)).await.unwrap();
   }
}