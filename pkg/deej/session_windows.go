package deej

import (
	"errors"
	"fmt"
	"strings"
	"time"

	wca "github.com/DarkMetalMouse/go-wca/pkg/wca"
	ole "github.com/go-ole/go-ole"
	"github.com/micmonay/keybd_event"
	ps "github.com/mitchellh/go-ps"
	"go.uber.org/zap"
)

var errNoSuchProcess = errors.New("No such process")
var errRefreshSessions = errors.New("Trigger session refresh")
var kb keybd_event.KeyBonding
var lastTime = time.Now()

type wcaSession struct {
	baseSession

	pid         uint32
	processName string

	control    *wca.IAudioSessionControl2
	volume     *wca.ISimpleAudioVolume
	audioLevel *wca.IAudioMeterInformation

	eventCtx *ole.GUID
}

type masterSession struct {
	baseSession

	volume     *wca.IAudioEndpointVolume
	audioLevel *wca.IAudioMeterInformation

	eventCtx *ole.GUID

	stale bool // when set to true, we should refresh sessions on the next call to SetVolume
}

func newWCASession(
	logger *zap.SugaredLogger,
	control *wca.IAudioSessionControl2,
	volume *wca.ISimpleAudioVolume,
	audioLevel *wca.IAudioMeterInformation,
	pid uint32,
	eventCtx *ole.GUID,
) (*wcaSession, error) {

	s := &wcaSession{
		control:    control,
		volume:     volume,
		audioLevel: audioLevel,
		pid:        pid,
		eventCtx:   eventCtx,
	}

	// special treatment for system sounds session
	if pid == 0 {
		s.system = true
		s.name = systemSessionName
		s.humanReadableDesc = "system sounds"
	} else {

		// find our session's process name
		process, err := ps.FindProcess(int(pid))
		if err != nil {
			logger.Warnw("Failed to find process name by ID", "pid", pid, "error", err)
			defer s.Release()

			return nil, fmt.Errorf("find process name by pid: %w", err)
		}

		// this PID may be invalid - this means the process has already been
		// closed and we shouldn't create a session for it.
		if process == nil {
			logger.Debugw("Process already exited, not creating audio session", "pid", pid)
			return nil, errNoSuchProcess
		}

		s.processName = process.Executable()
		s.name = s.processName
		s.humanReadableDesc = fmt.Sprintf("%s (pid %d)", s.processName, s.pid)
	}

	// use a self-identifying session name e.g. deej.sessions.chrome
	s.logger = logger.Named(strings.TrimSuffix(s.Key(), ".exe"))
	s.logger.Debugw(sessionCreationLogMessage, "session", s)

	return s, nil
}

func newMasterSession(
	logger *zap.SugaredLogger,
	volume *wca.IAudioEndpointVolume,
	audioLevel *wca.IAudioMeterInformation,
	eventCtx *ole.GUID,
	key string,
	loggerKey string,
) (*masterSession, error) {
	var err error
	kb, err = keybd_event.NewKeyBonding()
	if err != nil {
		panic(err)
	}

	s := &masterSession{
		volume:     volume,
		audioLevel: audioLevel,
		eventCtx:   eventCtx,
	}

	s.logger = logger.Named(loggerKey)
	s.master = true
	s.name = key
	s.humanReadableDesc = key

	s.logger.Debugw(sessionCreationLogMessage, "session", s)

	return s, nil
}

func (s *wcaSession) GetVolume() float32 {
	var level float32

	if err := s.volume.GetMasterVolume(&level); err != nil {
		s.logger.Warnw("Failed to get session volume", "error", err)
	}

	return level
}

func (s *wcaSession) SetVolume(v float32) error {
	if err := s.volume.SetMasterVolume(v, s.eventCtx); err != nil {
		s.logger.Warnw("Failed to set session volume", "error", err)
		return fmt.Errorf("adjust session volume: %w", err)
	}

	// mitigate expired sessions by checking the state whenever we change volumes
	var state uint32

	if err := s.control.GetState(&state); err != nil {
		s.logger.Warnw("Failed to get session state while setting volume", "error", err)
		return fmt.Errorf("get session state: %w", err)
	}

	if state == wca.AudioSessionStateExpired {
		s.logger.Warnw("Audio session expired, triggering session refresh")
		return errRefreshSessions
	}

	s.logger.Debugw("Adjusting session volume", "to", fmt.Sprintf("%.2f", v))

	return nil
}

func (s *wcaSession) GetMute() bool {
	var isMuted bool

	if err := s.volume.GetMute(&isMuted); err != nil {
		s.logger.Warnw("Failed to get session mute", "error", err)
	}

	return isMuted
}

func (s *wcaSession) SetMute(mute bool) error {
	if err := s.volume.SetMute(mute, s.eventCtx); err != nil {
		s.logger.Warnw("Failed to set session mute", "error", err)
		return fmt.Errorf("set session mute: %w", err)
	}

	// mitigate expired sessions by checking the state whenever we change mute
	var state uint32

	if err := s.control.GetState(&state); err != nil {
		s.logger.Warnw("Failed to get session state while setting mute", "error", err)
		return fmt.Errorf("get session state: %w", err)
	}

	if state == wca.AudioSessionStateExpired {
		s.logger.Warnw("Audio session expired, triggering session refresh")
		return errRefreshSessions
	}

	s.logger.Debugw("Setting session mute", "to", fmt.Sprintf("%t", mute))

	return nil
}

func (s *wcaSession) GetAudioLevel() float32 {
	var level float32

	if err := s.audioLevel.GetPeakValue(&level); err != nil {
		s.logger.Warnw("Failed to get session audio level", "error", err)
	}

	return level

}

func (s *wcaSession) Release() {
	s.logger.Debug("Releasing audio session")

	s.volume.Release()
	s.control.Release()
}

func (s *wcaSession) String() string {
	return fmt.Sprintf(sessionStringFormat, s.humanReadableDesc, s.GetVolume())
}

func (s *masterSession) GetVolume() float32 {
	var level float32

	if err := s.volume.GetMasterVolumeLevelScalar(&level); err != nil {
		s.logger.Warnw("Failed to get session volume", "error", err)
	}

	return level
}

func (s *masterSession) SetVolume(v float32) error {
	if s.stale {
		s.logger.Warnw("Session expired because default device has changed, triggering session refresh")
		return errRefreshSessions
	}

	if err := s.volume.SetMasterVolumeLevelScalar(v, s.eventCtx); err != nil {
		s.logger.Warnw("Failed to set session volume",
			"error", err,
			"volume", v)

		return fmt.Errorf("adjust session volume: %w", err)
	}
	if time.Since(lastTime) > 1*time.Second {
		kb.SetKeys(keybd_event.VK_VOLUME_DOWN, keybd_event.VK_VOLUME_UP)
		// Press the selected keys
		err := kb.Launching()
		if err != nil {
			panic(err)
		}
		lastTime = time.Now()
	}

	s.logger.Debugw("Adjusting session volume", "to", fmt.Sprintf("%.2f", v))

	return nil
}

func (s *masterSession) GetMute() bool {
	var isMuted bool

	if err := s.volume.GetMute(&isMuted); err != nil {
		s.logger.Warnw("Failed to get session mute", "error", err)
	}

	return isMuted
}

func (s *masterSession) SetMute(mute bool) error {
	if s.stale {
		s.logger.Warnw("Session expired because default device has changed, triggering session refresh")
		return errRefreshSessions
	}

	if err := s.volume.SetMute(mute, s.eventCtx); err != nil {
		s.logger.Warnw("Failed to set session mute", "error", err, "mute", mute)

		return fmt.Errorf("set session mute: %w", err)
	}

	s.logger.Debugw("Setting session mute", "to", fmt.Sprintf("%t", mute))

	return nil
}

func (s *masterSession) GetAudioLevel() float32 {
	var level float32

	if err := s.audioLevel.GetPeakValue(&level); err != nil {
		s.logger.Warnw("Failed to get session audio level", "error", err)
	}

	return level
}

func (s *masterSession) Release() {
	s.logger.Debug("Releasing audio session")

	s.volume.Release()
}

func (s *masterSession) String() string {
	return fmt.Sprintf(sessionStringFormat, s.humanReadableDesc, s.GetVolume())
}

func (s *masterSession) markAsStale() {
	s.stale = true
}
