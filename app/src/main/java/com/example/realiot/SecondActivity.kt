package com.example.realiot

import androidx.appcompat.app.AppCompatActivity
import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import java.io.IOException
import java.net.InetSocketAddress
import java.nio.ByteBuffer
import java.nio.channels.SocketChannel

class SecondActivity : AppCompatActivity() {

    private lateinit var socketChannel: SocketChannel

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_second)

        GlobalScope.launch(Dispatchers.IO) {
            try {
                socketChannel = SocketChannel.open()
                val serverAddress = InetSocketAddress(SERVER_IP, SERVER_PORT)

                socketChannel.connect(serverAddress)

                Log.d("SecondActivity", "Connected to server.")

                // 데이터 수신
                val buffer = ByteBuffer.allocate(1024)
                while (socketChannel.read(buffer) != -1) {
                    buffer.flip()
                    val receivedData = ByteArray(buffer.remaining())
                    buffer.get(receivedData)
                    val receivedMessage = String(receivedData)
                    handleReceivedMessage(receivedMessage)
                    buffer.clear()
                }

            } catch (e: IOException) {
                e.printStackTrace()
            } finally {
                socketChannel.close()
            }
        }

    }

    override fun onDestroy() {
        super.onDestroy()
        // 소켓 채널 닫기
        try {
            socketChannel.close()
        } catch (e: IOException) {
            e.printStackTrace()
        }
    }


    private fun handleReceivedMessage(receivedMessage: String) {
        runOnUiThread {
            Log.d("SecondActivity", "Received data: $receivedMessage")

            // 메시지 분리 및 처리
            val messages = receivedMessage.split("\n")
            for (message in messages) {
                if (message.isNotEmpty()) {
                    when (message) {
                        "fire" -> {
                            val intent2 = Intent(this@SecondActivity, FireActivity::class.java)
                            startActivity(intent2)
                        }
                        "ms" -> Toast.makeText(this@SecondActivity, "움직임과 소리 감지!", Toast.LENGTH_SHORT).show()
                        "m" -> Toast.makeText(this@SecondActivity, "움직임 감지!", Toast.LENGTH_SHORT).show()
                        "s" -> Toast.makeText(this@SecondActivity, "소리 감지!", Toast.LENGTH_SHORT).show()
                    }
                }
            }
        }
    }

    companion object {
        private const val SERVER_IP = "172.20.48.142"
        private const val SERVER_PORT = 8886
    }
}