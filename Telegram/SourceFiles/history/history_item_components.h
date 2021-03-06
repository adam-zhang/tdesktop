/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "history/history_item.h"
#include "base/value_ordering.h"

class HistoryDocument;
class HistoryWebPage;

struct MessageGroupId {
	using Underlying = uint64;

	enum Type : Underlying {
		None = 0,
	} value;

	MessageGroupId(Type value = None) : value(value) {
	}
	static MessageGroupId FromRaw(Underlying value) {
		return static_cast<Type>(value);
	}

	explicit operator bool() const {
		return value != None;
	}
	Underlying raw() const {
		return static_cast<Underlying>(value);
	}

	friend inline Type value_ordering_helper(MessageGroupId value) {
		return value.value;
	}

};

struct HistoryMessageVia : public RuntimeComponent<HistoryMessageVia> {
	void create(UserId userId);
	void resize(int32 availw) const;

	UserData *bot = nullptr;
	mutable QString text;
	mutable int width = 0;
	mutable int maxWidth = 0;
	ClickHandlerPtr link;
};

struct HistoryMessageViews : public RuntimeComponent<HistoryMessageViews> {
	QString _viewsText;
	int _views = 0;
	int _viewsWidth = 0;
};

struct HistoryMessageSigned : public RuntimeComponent<HistoryMessageSigned> {
	void refresh(const QString &date);
	int maxWidth() const;

	QString author;
	Text signature;
};

struct HistoryMessageEdited : public RuntimeComponent<HistoryMessageEdited> {
	void refresh(const QString &date, bool displayed);
	int maxWidth() const;

	QDateTime date;
	Text text;
};

struct HistoryMessageForwarded : public RuntimeComponent<HistoryMessageForwarded> {
	void create(const HistoryMessageVia *via) const;

	QDateTime originalDate;
	PeerData *originalSender = nullptr;
	QString originalAuthor;
	MsgId originalId = 0;
	mutable Text text = { 1 };

	PeerData *savedFromPeer = nullptr;
	MsgId savedFromMsgId = 0;
};

struct HistoryMessageReply : public RuntimeComponent<HistoryMessageReply> {
	HistoryMessageReply() = default;
	HistoryMessageReply(const HistoryMessageReply &other) = delete;
	HistoryMessageReply(HistoryMessageReply &&other) = delete;
	HistoryMessageReply &operator=(const HistoryMessageReply &other) = delete;
	HistoryMessageReply &operator=(HistoryMessageReply &&other) {
		replyToMsgId = other.replyToMsgId;
		std::swap(replyToMsg, other.replyToMsg);
		replyToLnk = std::move(other.replyToLnk);
		replyToName = std::move(other.replyToName);
		replyToText = std::move(other.replyToText);
		replyToVersion = other.replyToVersion;
		maxReplyWidth = other.maxReplyWidth;
		replyToVia = std::move(other.replyToVia);
		return *this;
	}
	~HistoryMessageReply() {
		// clearData() should be called by holder.
		Expects(replyToMsg == nullptr);
		Expects(replyToVia == nullptr);
	}

	bool updateData(HistoryMessage *holder, bool force = false);

	// Must be called before destructor.
	void clearData(HistoryMessage *holder);

	bool isNameUpdated() const;
	void updateName() const;
	void resize(int width) const;
	void itemRemoved(HistoryMessage *holder, HistoryItem *removed);

	enum class PaintFlag {
		InBubble = (1 << 0),
		Selected = (1 << 1),
	};
	using PaintFlags = base::flags<PaintFlag>;
	friend inline constexpr auto is_flag_type(PaintFlag) { return true; };
	void paint(
		Painter &p,
		const HistoryItem *holder,
		int x,
		int y,
		int w,
		PaintFlags flags) const;

	MsgId replyToId() const {
		return replyToMsgId;
	}
	int replyToWidth() const {
		return maxReplyWidth;
	}
	ClickHandlerPtr replyToLink() const {
		return replyToLnk;
	}

	MsgId replyToMsgId = 0;
	HistoryItem *replyToMsg = nullptr;
	ClickHandlerPtr replyToLnk;
	mutable Text replyToName, replyToText;
	mutable int replyToVersion = 0;
	mutable int maxReplyWidth = 0;
	std::unique_ptr<HistoryMessageVia> replyToVia;
	int toWidth = 0;

};

struct HistoryMessageMarkupButton {
	enum class Type {
		Default,
		Url,
		Callback,
		RequestPhone,
		RequestLocation,
		SwitchInline,
		SwitchInlineSame,
		Game,
		Buy,
	};
	Type type;
	QString text;
	QByteArray data;
	mutable mtpRequestId requestId;

};

struct HistoryMessageReplyMarkup : public RuntimeComponent<HistoryMessageReplyMarkup> {
	using Button = HistoryMessageMarkupButton;

	HistoryMessageReplyMarkup() = default;
	HistoryMessageReplyMarkup(MTPDreplyKeyboardMarkup::Flags f) : flags(f) {
	}

	void create(const MTPReplyMarkup &markup);
	void create(const HistoryMessageReplyMarkup &markup);

	std::vector<std::vector<Button>> rows;
	MTPDreplyKeyboardMarkup::Flags flags = 0;

	std::unique_ptr<ReplyKeyboard> inlineKeyboard;

	// If >= 0 it holds the y coord of the inlineKeyboard before the last edition.
	int oldTop = -1;

private:
	void createFromButtonRows(const QVector<MTPKeyboardButtonRow> &v);

};

class ReplyMarkupClickHandler : public LeftButtonClickHandler {
public:
	ReplyMarkupClickHandler(int row, int column, FullMsgId context);

	QString tooltip() const override {
		return _fullDisplayed ? QString() : buttonText();
	}

	void setFullDisplayed(bool full) {
		_fullDisplayed = full;
	}

	// Copy to clipboard support.
	void copyToClipboard() const override;
	QString copyToClipboardContextItemText() const override;

	// Finds the corresponding button in the items markup struct.
	// If the button is not found it returns nullptr.
	// Note: it is possible that we will point to the different button
	// than the one was used when constructing the handler, but not a big deal.
	const HistoryMessageMarkupButton *getButton() const;

	// We hold only FullMsgId, not HistoryItem*, because all click handlers
	// are activated async and the item may be already destroyed.
	void setMessageId(const FullMsgId &msgId) {
		_itemId = msgId;
	}

protected:
	void onClickImpl() const override;

private:
	FullMsgId _itemId;
	int _row = 0;
	int _column = 0;
	bool _fullDisplayed = true;

	// Returns the full text of the corresponding button.
	QString buttonText() const;

};

class ReplyKeyboard {
private:
	struct Button;

public:
	class Style {
	public:
		Style(const style::BotKeyboardButton &st) : _st(&st) {
		}

		virtual void startPaint(Painter &p) const = 0;
		virtual const style::TextStyle &textStyle() const = 0;

		int buttonSkip() const;
		int buttonPadding() const;
		int buttonHeight() const;
		virtual int buttonRadius() const = 0;

		virtual void repaint(not_null<const HistoryItem*> item) const = 0;
		virtual ~Style() {
		}

	protected:
		virtual void paintButtonBg(
			Painter &p,
			const QRect &rect,
			float64 howMuchOver) const = 0;
		virtual void paintButtonIcon(
			Painter &p,
			const QRect &rect,
			int outerWidth,
			HistoryMessageMarkupButton::Type type) const = 0;
		virtual void paintButtonLoading(
			Painter &p,
			const QRect &rect) const = 0;
		virtual int minButtonWidth(
			HistoryMessageMarkupButton::Type type) const = 0;

	private:
		const style::BotKeyboardButton *_st;

		void paintButton(Painter &p, int outerWidth, const ReplyKeyboard::Button &button, TimeMs ms) const;
		friend class ReplyKeyboard;

	};

	ReplyKeyboard(
		not_null<const HistoryItem*> item,
		std::unique_ptr<Style> &&s);
	ReplyKeyboard(const ReplyKeyboard &other) = delete;
	ReplyKeyboard &operator=(const ReplyKeyboard &other) = delete;

	bool isEnoughSpace(int width, const style::BotKeyboardButton &st) const;
	void setStyle(std::unique_ptr<Style> &&s);
	void resize(int width, int height);

	// what width and height will best fit this keyboard
	int naturalWidth() const;
	int naturalHeight() const;

	void paint(Painter &p, int outerWidth, const QRect &clip, TimeMs ms) const;
	ClickHandlerPtr getState(QPoint point) const;

	void clickHandlerActiveChanged(const ClickHandlerPtr &p, bool active);
	void clickHandlerPressedChanged(const ClickHandlerPtr &p, bool pressed);

	void clearSelection();
	void updateMessageId();

private:
	friend class Style;
	struct Button {
		Button();
		Button(Button &&other);
		Button &operator=(Button &&other);
		~Button();

		Text text = { 1 };
		QRect rect;
		int characters = 0;
		float64 howMuchOver = 0.;
		HistoryMessageMarkupButton::Type type;
		std::shared_ptr<ReplyMarkupClickHandler> link;
		mutable std::unique_ptr<Ui::RippleAnimation> ripple;
	};
	struct ButtonCoords {
		int i, j;
	};

	void startAnimation(int i, int j, int direction);

	ButtonCoords findButtonCoordsByClickHandler(const ClickHandlerPtr &p);

	void step_selected(TimeMs ms, bool timer);

	const not_null<const HistoryItem*> _item;
	int _width = 0;

	std::vector<std::vector<Button>> _rows;

	base::flat_map<int, TimeMs> _animations;
	BasicAnimation _a_selected;
	std::unique_ptr<Style> _st;

	ClickHandlerPtr _savedPressed;
	ClickHandlerPtr _savedActive;
	mutable QPoint _savedCoords;

};

// Any HistoryItem can have this Component for
// displaying the day mark above the message.
struct HistoryMessageDate : public RuntimeComponent<HistoryMessageDate> {
	void init(const QDateTime &date);

	int height() const;
	void paint(Painter &p, int y, int w) const;

	QString _text;
	int _width = 0;
};

// Any HistoryItem can have this Component for
// displaying the unread messages bar above the message.
struct HistoryMessageUnreadBar : public RuntimeComponent<HistoryMessageUnreadBar> {
	void init(int count);

	static int height();
	static int marginTop();

	void paint(Painter &p, int y, int w) const;

	QString _text;
	int _width = 0;

	// If unread bar is freezed the new messages do not
	// increment the counter displayed by this bar.
	//
	// It happens when we've opened the conversation and
	// we've seen the bar and new messages are marked as read
	// as soon as they are added to the chat history.
	bool _freezed = false;

};

struct HistoryMessageGroup : public RuntimeComponent<HistoryMessageGroup> {
	MessageGroupId groupId = MessageGroupId::None;
	HistoryItem *leader = nullptr;
	std::vector<not_null<HistoryItem*>> others;
};

// Special type of Component for the channel actions log.
struct HistoryMessageLogEntryOriginal : public RuntimeComponent<HistoryMessageLogEntryOriginal> {
	HistoryMessageLogEntryOriginal();
	HistoryMessageLogEntryOriginal(HistoryMessageLogEntryOriginal &&other);
	HistoryMessageLogEntryOriginal &operator=(HistoryMessageLogEntryOriginal &&other);
	~HistoryMessageLogEntryOriginal();

	std::unique_ptr<HistoryWebPage> _page;

};

struct HistoryDocumentThumbed : public RuntimeComponent<HistoryDocumentThumbed> {
	ClickHandlerPtr _linksavel, _linkcancell;
	int _thumbw = 0;

	mutable int _linkw = 0;
	mutable QString _link;
};

struct HistoryDocumentCaptioned : public RuntimeComponent<HistoryDocumentCaptioned> {
	HistoryDocumentCaptioned();

	Text _caption;
};

struct HistoryDocumentNamed : public RuntimeComponent<HistoryDocumentNamed> {
	QString _name;
	int _namew = 0;
};

struct HistoryDocumentVoicePlayback {
	HistoryDocumentVoicePlayback(const HistoryDocument *that);

	int32 _position = 0;
	anim::value a_progress;
	BasicAnimation _a_progress;
};

class HistoryDocumentVoice : public RuntimeComponent<HistoryDocumentVoice> {
	// We don't use float64 because components should align to pointer even on 32bit systems.
	static constexpr float64 kFloatToIntMultiplier = 65536.;

public:
	void ensurePlayback(const HistoryDocument *interfaces) const;
	void checkPlaybackFinished() const;

	mutable std::unique_ptr<HistoryDocumentVoicePlayback> _playback;
	std::shared_ptr<VoiceSeekClickHandler> _seekl;
	mutable int _lastDurationMs = 0;

	bool seeking() const {
		return _seeking;
	}
	void startSeeking();
	void stopSeeking();
	float64 seekingStart() const {
		return _seekingStart / kFloatToIntMultiplier;
	}
	void setSeekingStart(float64 seekingStart) const {
		_seekingStart = qRound(seekingStart * kFloatToIntMultiplier);
	}
	float64 seekingCurrent() const {
		return _seekingCurrent / kFloatToIntMultiplier;
	}
	void setSeekingCurrent(float64 seekingCurrent) {
		_seekingCurrent = qRound(seekingCurrent * kFloatToIntMultiplier);
	}

private:
	bool _seeking = false;

	mutable int _seekingStart = 0;
	mutable int _seekingCurrent = 0;

};
