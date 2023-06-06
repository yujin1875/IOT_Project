package com.example.realiot


import android.content.Intent
import android.os.Bundle
import android.os.Handler
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {
    private val DELAY_MS: Long = 1500 // 1.5초

    private var handler: Handler? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        handler = Handler()

        // 1.5초 후에 두 번째 액티비티로 전환
        handler?.postDelayed({
            val intent = Intent(this@MainActivity, SecondActivity::class.java)
            startActivity(intent)
        }, DELAY_MS)
    }

    override fun onDestroy() {
        super.onDestroy()
        // 핸들러가 동작 중인 경우에는 핸들러 작업을 취소
        handler?.removeCallbacksAndMessages(null)
    }
}
