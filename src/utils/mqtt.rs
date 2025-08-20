
use tokio::{sync::{mpsc::Sender}, time::Duration};
use futures_util::StreamExt;
use paho_mqtt;


pub fn spawn_mqtt(tx:Sender<String>){
    let _=tokio::spawn(async move{
        let mut pre_data:String=String::from("");
        let opt = paho_mqtt::CreateOptionsBuilder::new()
        .client_id("as")
        .server_uri("mqtt://broker.mqtt.cool:1883")
        .finalize();

    let mut cli = paho_mqtt::AsyncClient::new(opt).unwrap();

    let lwt = paho_mqtt::MessageBuilder::new()
        .qos(0)
        .topic("/dht/aryan")
        .finalize();

    let conopt = paho_mqtt::ConnectOptionsBuilder::new()
        .keep_alive_interval(Duration::from_secs(5))
        .clean_session(false)
        .will_message(lwt)
        .finalize();

    cli.connect(conopt).await.unwrap();
    cli.subscribe("/dht/aryan", 0).await.unwrap();

    let mut strm = cli.get_stream(25);

    while let Some(optmsg) = strm.next().await {
        if let Some(msg) = optmsg {
            
            if msg.payload_str()!=pre_data{
                println!("Got: {}", msg.payload_str());
                pre_data=msg.payload_str().to_string();
                tx.send(msg.to_string()).await.unwrap();
            }
        } else {
            println!("Lost connection.");
            break;
        }
    }
    });
}