package com.example.realiot

import android.graphics.Color
import android.os.Handler
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.Button
import android.media.AudioManager
import android.media.ToneGenerator



class FireActivity : AppCompatActivity() {
    private val handler = Handler()
    private var currentColor = Color.YELLOW
    private lateinit var fireView: View
    private lateinit var toneGenerator: ToneGenerator

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.fire)
        fireView = findViewById<View>(R.id.fireView)
        toneGenerator = ToneGenerator(AudioManager.STREAM_ALARM, 100)

        startBlinkingBackground()
        startBeepingSound()

        val closeButton = findViewById<Button>(R.id.button)
        closeButton.setOnClickListener {
            finish()
        }

    }

    private fun startBlinkingBackground() {
        // 반복적으로 배경색을 변경하는 Runnable
        val changeColorRunnable = object : Runnable {
            override fun run() {
                if (currentColor == Color.YELLOW) {
                    fireView.setBackgroundColor(Color.RED)
                    currentColor = Color.RED
                } else {
                    fireView.setBackgroundColor(Color.YELLOW)
                    currentColor = Color.YELLOW
                }

                // 0.5초 후에 Runnable을 다시 실행
                handler.postDelayed(this, 500)
            }
        }

        // 처음에 0.5초 후에 Runnable을 실행
        handler.postDelayed(changeColorRunnable, 500)
    }

    private fun startBeepingSound() {
        val beepRunnable = object : Runnable {
            override fun run() {
                val toneGenerator = ToneGenerator(AudioManager.STREAM_ALARM, 10000) // 볼륨 값을 1000으로 설정
                toneGenerator.startTone(ToneGenerator.TONE_CDMA_ALERT_CALL_GUARD, 300) // 0.3초 동안 삐 소리 재생

                // 0.3초 후에 Runnable을 다시 실행
                handler.postDelayed(this, 300)
            }
        }

        // 처음에 0.3초 후에 Runnable을 실행
        handler.postDelayed(beepRunnable, 300)
    }

    override fun onDestroy() {
        super.onDestroy()
        // 핸들러가 더 이상 실행되지 않도록 제거
        handler.removeCallbacksAndMessages(null)
    }
}